// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"

#include "ospcommon/FileName.h"
#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"
// std
#include <mutex>

namespace ospray {
  namespace amr {
    using namespace ospcommon::array3D;
    using namespace ospcommon;
    using namespace std;

    struct BrickDesc
    {
      box3i box;
      int level;
      float dt;
    };

    static std::mutex fileMutex;
    static FILE *infoOut = nullptr;
    static FILE *dataOut = nullptr;

    static size_t numWritten = 0;
    static size_t numRemoved = 0;

    struct Progress
    {
      Progress(const char *message, size_t numTotal, float pingInterval = 10.f)
          : numTotal(numTotal),
            pingInterval(pingInterval),
            message(message)
      {
      }

      void ping()
      {
        std::lock_guard<std::mutex> lock(mutex);
        double t = getSysTime();
        numDone++;
        if (t - lastPingTime > pingInterval) {
          float pct = float(numDone * 100.f / numTotal);
          printf("%s : %li/%li (%.3f%%), removed %li/%li (%.2f%%)\n",
                 message.c_str(),
                 numDone,
                 numTotal,
                 pct,
                 numRemoved,
                 numRemoved + numWritten,
                 numRemoved * 100.f / (numRemoved + numWritten));
          lastPingTime = t;
        }
      }

      size_t numDone {0};
      size_t numTotal;
      float pingInterval;
      std::mutex mutex;
      std::string message;
      double lastPingTime {-1.0};
    };

    void makeAMR(const std::shared_ptr<Array3D<float>> _in,
                 const int numLevels,
                 const int BS,
                 const int RF,
                 const float threshold)
    {
      std::shared_ptr<Array3D<float>> in = _in;

      int minWidth = BS;
      std::cout << "building AMR model out of RAW volume. BS = " << BS
                << ", RF = " << RF << endl;

      for (int i = 1; i < numLevels; i++)
        minWidth *= RF;

      if (minWidth >= RF * reduce_max(in->size()))
        throw std::runtime_error(
            "too many levels, or too fine a refinement factor."
            "do not have a single brick at the root...");
      vec3i finestLevelSize = ospcommon::vec3i(minWidth);
      while (finestLevelSize.x < in->size().x)
        finestLevelSize.x += minWidth;
      while (finestLevelSize.y < in->size().y)
        finestLevelSize.y += minWidth;
      while (finestLevelSize.z < in->size().z)
        finestLevelSize.z += minWidth;

      cout << "logical finest level size is " << finestLevelSize << endl;
      cout << "(note: input size was " << in->size() << ")" << endl;

      /* create and write the bricks */
      vec3i levelSize = finestLevelSize;
      for (int level = numLevels - 1; level >= 0; --level) {
        cout << "-------------------------------------------------------"
             << endl;
        cout << "logical size of level " << level << " is " << levelSize
             << endl;
        const vec3i nextLevelSize = levelSize / RF;
        cout << "reducing to next level of " << nextLevelSize << endl;
        std::shared_ptr<ActualArray3D<float>> nextLevel =
            std::make_shared<ActualArray3D<float>>(nextLevelSize);
        // clear next level
        for (int iz = 0; iz < nextLevelSize.z; iz++)
          for (int iy = 0; iy < nextLevelSize.y; iy++)
            for (int ix = 0; ix < nextLevelSize.x; ix++)
              nextLevel->set(vec3i(ix, iy, iz), 0.f);

        const vec3i numBricks = levelSize / BS;
        PRINT(numBricks);
        PRINT(numBricks.product());
        Progress progress("level progress:", numBricks.product(), 30.f);
        ospcommon::tasking::parallel_for(
            numBricks.product(), [&](int brickIdx) {
              BrickDesc brick;
              brick.level = level;
              brick.dt    = 1.f / powf(RF, level);
              const vec3i brickID(brickIdx % numBricks.x,
                                  (brickIdx / numBricks.x) % numBricks.y,
                                  brickIdx / (numBricks.x * numBricks.y));
              brick.box.lower = brickID * BS;
              brick.box.upper = brick.box.lower + (BS - 1);
              std::vector<float> data(BS * BS * BS);
              size_t out = 0;
              ospcommon::range1f brickRange;
              for (int iz = brick.box.lower.z; iz <= brick.box.upper.z; iz++)
                for (int iy = brick.box.lower.y; iy <= brick.box.upper.y; iy++)
                  for (int ix = brick.box.lower.x; ix <= brick.box.upper.x;
                       ix++) {
                    const vec3i thisLevelCoord(ix, iy, iz);
                    const vec3i nextLevelCoord(ix / RF, iy / RF, iz / RF);
                    const float v = in->get(thisLevelCoord);
                    data[out++]   = v;
                    nextLevel->set(
                        nextLevelCoord,
                        nextLevel->get(nextLevelCoord) + (v / (RF * RF * RF)));
                    brickRange.extend(v);
                  }

              std::lock_guard<std::mutex> lock(fileMutex);
              if ((level > 0) &&
                  ((brickRange.upper - brickRange.lower) <= threshold)) {
                numRemoved++;
              } else {
                numWritten++;
                fwrite(&brick, sizeof(brick), 1, infoOut);
                fwrite(data.data(), sizeof(float), BS * BS * BS, dataOut);
              }
              progress.ping();
            });
        in        = nextLevel;
        levelSize = nextLevelSize;
        cout << "done level " << level << ", written " << numWritten
             << " bricks, removed " << numRemoved << endl;
      }
    }

    extern "C" int main(int ac, char **av)
    {
      if (ac != 11) {
        cout << "usage: ./raw2amr in.raw <float|byte|double> Nx Ny Nz "
                "numLevels brickSize refineFactor threshold outfilebase"
             << endl;
        exit(0);
      }

      const char *inFileName   = av[1];
      const std::string format = av[2];
      const vec3i inDims(atoi(av[3]), atoi(av[4]), atoi(av[5]));
      int numLevels           = atoi(av[6]);
      int BS                  = atoi(av[7]);
      int RF                  = atoi(av[8]);
      float threshold         = atof(av[9]);
      std::string outFileBase = av[10];

      std::shared_ptr<Array3D<float>> in;
      if (format == "float") {
        in = mmapRAW<float>(inFileName, inDims);
      } else if (format == "byte" || format == "uchar" || format == "uint8") {
        in = std::make_shared<Array3DAccessor<unsigned char, float>>(
            mmapRAW<unsigned char>(inFileName, inDims));
      } else if (format == "double" || format == "float64") {
        in = std::make_shared<Array3DAccessor<double, float>>(
            mmapRAW<double>(inFileName, inDims));
      } else {
        throw std::runtime_error("unknown input voxel format");
      }

      infoOut = fopen((outFileBase + ".info").c_str(), "wb");
      if (!infoOut)
        throw std::runtime_error("could not open info output file!");

      dataOut = fopen((outFileBase + ".data").c_str(), "wb");
      if (!dataOut)
        throw std::runtime_error("could not open data output file!");

      ofstream osp(outFileBase + ".osp");
      osp << "<?xml?>" << endl;
      osp << "<ospray>" << endl;
      osp << "<TransferFunction name=\"xf\">" << endl;
      osp << "    <alpha>" << endl;
      osp << "      0 0" << endl;
      osp << "      0.3 0" << endl;
      osp << "      0.3 1" << endl;
      osp << "      0.8 1" << endl;
      osp << "      0.8 0" << endl;
      osp << "      1 0" << endl;
      osp << "    </alpha>" << endl;
      osp << "  </TransferFunction>" << endl;
      osp << "	<AMRVolume" << endl;
      osp << "	fileName=\"" << ospcommon::FileName(outFileBase).base() << "\""
          << endl;
      osp << "        transferFunction=\"xf\"" << endl;
      osp << "        brickSize=\"" << BS << "\"" << endl;
      osp << "	clamp=\"0 100000\"" << endl;
      osp << "	/>" << endl;
      osp << "</ospray>" << endl;

      makeAMR(in, numLevels, BS, RF, threshold);

      fclose(infoOut);
      fclose(dataOut);
      return 0;
    }
  }  // namespace amr
}  // namespace ospray

#pragma clang diagnostic pop
