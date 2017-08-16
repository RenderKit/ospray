// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

/*! \file sg/AMRVolume.cpp node for reading and rendering amr files */

#undef NDEBUG

#include "AMRVolume.h"
// sg
#include "sg/importer/Importer.h"

#define amr_ISO_SURFACE 0
#define amr_NO_VOLUME 0

namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;

    struct Histogram
    {
      enum
      {
        N = 128
      };

      Histogram(const Range<float> &range) : range(range)
      {
        for (int i = 0; i < N; i++)
          bin[i]   = 0;
      }
      inline void count(float f)
      {
        int i = int(N * (f - range.lower) / (range.upper - range.lower));
        if (i == N)
          i = N - 1;
        if (i < 0 || i >= N) {
          PRINT(f);
          PRINT(range);
          PRINT(i);
        }
        assert(i >= 0);
        assert(i < N);
        bin[i]++;
        total++;
      }
      void print()
      {
        for (int i = 0; i < N; i++) {
          float lo = range.lower + (range.upper - range.lower) * float(i) / N;
          float hi =
              range.lower + (range.upper - range.lower) * float(i + 1) / N;
          printf("bin[%3i] { %8.4f-%8.4f } = %9li (%3i%%)\n",
                 i,
                 lo,
                 hi,
                 bin[i],
                 int(bin[i] * 100.f / total));
        }
      };
      Range<float> range;
      size_t bin[N];
      size_t total;
    };

    void importAMR(const FileName &fileName, sg::ImportState &importerState)
    {
#ifdef OSPRAY_APPS_SG_CHOMBO
      std::shared_ptr<AMRVolume> cv = std::make_shared<AMRVolume>();
      parseAMRChomboFile(cv, fileName, "", NULL);
      importerState.world->add(cv);
#endif
    }

    inline float clamp(const Range<float> &range, float v)
    {
      return max(range.lower, min(range.upper, v));
    }

    void AMRVolume::parseRaw2AmrFile(const FileName &fileName,
                                     int BS,
                                     int maxLevel)
    {
      char *maxLevelEnv = getenv("AMR_MAX_LEVEL");
      if (maxLevelEnv) {
        maxLevel = atoi(maxLevelEnv);
        ospLogF(1) << "will only parse amr file up to level " << maxLevel
                   << std::endl;
      }

      FileName infoFileName = fileName.str() + std::string(".info");
      FileName dataFileName = fileName.str() + std::string(".data");
      FILE *infoFile        = fopen(infoFileName.c_str(), "rb");
      assert(infoFile);
      FILE *dataFile = fopen(dataFileName.c_str(), "rb");
      assert(dataFile);

      BrickInfo bi;
      auto bounds = child("bounds").valueAs<box3f>();
      while (fread(&bi, sizeof(bi), 1, infoFile)) {
        float *bd = new float[BS * BS * BS];
        int nr    = fread(bd, sizeof(float), BS * BS * BS, dataFile);
        if (bi.level > maxLevel) {
          delete bd;
          continue;
        }
        brickInfo.push_back(bi);
        bounds.extend((vec3f(bi.box.upper) + vec3f(1.f)) * bi.dt);

        assert(nr == BS * BS * BS);
        for (int i = 0; i < BS * BS * BS; i++)
          valueRange.extend(bd[i]);
        brickPtrs.push_back(bd);
      }
      child("bounds").setValue(bounds);
      ospLogF(1) << "read file; found " << brickInfo.size() << " bricks"
                 << endl;

      fclose(infoFile);
      fclose(dataFile);
    }

    void AMRVolume::preCommit(RenderContext &ctx)
    {
      OSPObject volume = valueAs<OSPObject>();
      if (volume != NULL)
        return;

      assert(!brickInfo.empty());
      for (int bID = 0; bID < brickInfo.size(); bID++) {
        BrickInfo bi = brickInfo[bID];

        OSPData data = ospNewData(bi.size().product(),
                                  OSP_FLOAT,
                                  this->brickPtrs[bID],
                                  OSP_DATA_SHARED_BUFFER);
        this->brickData.push_back(data);
      }

      volume = ospNewVolume("amr_volume");
      assert(volume);

      setValue((OSPObject)volume);

      brickDataData =
          ospNewData(brickData.size(), OSP_OBJECT, &brickData[0], 0);
      ospSetData(volume, "brickData", brickDataData);
      brickInfoData = ospNewData(
          brickInfo.size() * sizeof(brickInfo[0]), OSP_RAW, &brickInfo[0], 0);
      ospSetData(volume, "brickInfo", brickInfoData);
      ospSet1i(volume, "singleShade", 1);
      ospSet1i(volume, "preIntegration", 0);
      ospSet1i(volume, "gradientShadingEnabled", 1);
      ospSet1i(volume, "adaptiveSampling", 1);
      ospSet2f(
          volume, "voxelRange", valueRange.toVec2f().x, valueRange.toVec2f().y);
      child("voxelRange").setValue(valueRange.toVec2f());

      child("transferFunction")["valueRange"].setValue(valueRange.toVec2f());
      child("transferFunction").preCommit(ctx);

      ospSetObject(volume,
                   "transferFunction",
                   child("transferFunction").valueAs<OSPObject>());

      ospCommit(volume);
      ospAddVolume((OSPModel)ctx.world->valueAs<OSPObject>(),
                   (OSPVolume)volume);
    }

    void AMRVolume::postCommit(RenderContext &ctx)
    {
      OSPObject volume = valueAs<OSPObject>();
      ospSetObject(volume,
                   "transferFunction",
                   child("transferFunction").valueAs<OSPObject>());
      ospCommit(volume);
    }

    //! \brief Initialize this node's value from given XML node
    void AMRVolume::setFromXML(const xml::Node &node,
                               const unsigned char *binBasePtr)
    {
      std::string fileName = node.getProp("fileName");
      Range<float> clampRange;
      std::string clampRangeString = node.getProp("clamp");
      if (!clampRangeString.empty()) {
        sscanf(clampRangeString.c_str(),
               "%f %f",
               &clampRange.lower,
               &clampRange.upper);
      }
      if (fileName != "") {
        const std::string compName = node.getProp("component");
        FileName realFN            = node.doc->fileName.path() + fileName;
        if (realFN.ext() == "hdf5") {
#ifdef OSPRAY_APPS_SG_CHOMBO
          auto nodePtr = nodeAs<AMRVolume>();
          parseAMRChomboFile(nodePtr,
                             realFN,
                             compName,
                             clampRangeString.empty() ? NULL : &clampRange,
                             child("maxLevel").valueAs<int>());
#else
          throw std::runtime_error("chombo support not built in");
#endif
        } else {
          std::string BSs = node.getProp("brickSize");
          if (BSs == "")
            throw std::runtime_error(
                "no 'brickSize' specified for raw2amr generated file");
          int BS = atoi(BSs.c_str());
          parseRaw2AmrFile(realFN, BS);
        }
      } else
        throw std::runtime_error("no filename set in xml node...");
    }

    OSP_REGISTER_SG_NODE(AMRVolume);
  }
}
