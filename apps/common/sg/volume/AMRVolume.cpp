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

#include "AMRVolume.h"
// sg
#include "sg/importer/Importer.h"

namespace ospray {
  namespace sg {

    AMRVolume::AMRVolume()
    {
      createChild("maxLevel", "int", 1 << 30);
      createChild("amrMethod", "string", std::string("current"))
          .setWhiteList({std::string("current"),
                         std::string("currentLevel"),
                         std::string("octant"),
                         std::string("finest"),
                         std::string("finestLevel")});
    }

    std::string AMRVolume::toString() const
    {
      return "ospray::sg::AMRVolume";
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

      FILE *infoFile = fopen(infoFileName.c_str(), "rb");
      FILE *dataFile = fopen(dataFileName.c_str(), "rb");

      if (infoFile == nullptr) {
        throw std::runtime_error(std::string("#osp:sg - ERROR could not open '")
                                 + infoFileName.c_str() + "'");
      }

      if (dataFile == nullptr) {
        throw std::runtime_error(std::string("#osp:sg - ERROR could not open '")
                                 + dataFileName.c_str() + "'");
      }

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

      child("bounds") = bounds;
      ospLogF(1) << "read file; found " << brickInfo.size() << " bricks"
                 << std::endl;

      fclose(infoFile);
      fclose(dataFile);
    }

    void AMRVolume::preCommit(RenderContext &ctx)
    {
      auto volume = valueAs<OSPVolume>();

      if (!volume) {
        volume = ospNewVolume("amr_volume");
        setValue(volume);
      }

      for (int bID = 0; bID < brickInfo.size(); bID++) {
        const auto &bi = brickInfo[bID];
        OSPData data = ospNewData(bi.size().product(),
                                  OSP_FLOAT,
                                  this->brickPtrs[bID],
                                  OSP_DATA_SHARED_BUFFER);
        this->brickData.push_back(data);
      }

      brickDataData =
          ospNewData(brickData.size(), OSP_OBJECT, brickData.data());
      ospSetData(volume, "brickData", brickDataData);
      brickInfoData = ospNewData(
          brickInfo.size() * sizeof(brickInfo[0]), OSP_RAW, brickInfo.data());
      ospSetData(volume, "brickInfo", brickInfoData);

      child("voxelRange") = valueRange.toVec2f();
    }

    void AMRVolume::setFromXML(const xml::Node &node,
                               const unsigned char *binBasePtr)
    {
      std::string fileName = node.getProp("fileName");
      range1f clampRange;
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
                             clampRangeString.empty() ? nullptr : &clampRange,
                             child("maxLevel").valueAs<int>());
#else
          throw std::runtime_error("chombo support not built in");
#endif
        } else {
          std::string BSs = node.getProp("brickSize");
          if (BSs == "") {
            throw std::runtime_error(
                "no 'brickSize' specified for raw2amr generated file");
          }
          int BS = atoi(BSs.c_str());
          parseRaw2AmrFile(realFN, BS);
        }
      } else
        throw std::runtime_error("no filename set in xml node...");

      std::string method = node.getProp("method");
      if (method.empty())
        method = node.getProp("amrMethod");

      if (!method.empty())
        child("amrMethod") = method;

      child("transferFunction")["valueRange"] = valueRange.toVec2f();
    }

    OSP_REGISTER_SG_NODE(AMRVolume);
  }
}
