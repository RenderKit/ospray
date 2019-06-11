// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "rawToAMR.h"

#include <errno.h>
#include <stdio.h>

namespace ospray {
    namespace amr {

        void makeAMR(const std::shared_ptr<Array3D<float>> in,
                     const int numLevels,
                     const int blockSize,
                     const int refinementLevel,
                     const float threshold,
                     std::vector<BrickDesc> &brickInfo,
                     std::vector<std::vector<float>> &brickData)
        {
            // minWidth starts with provided brick size
            int minWidth = blockSize;
            std::cout << "building AMR model out of RAW volume. blockSize = " << blockSize
                << ", refinementLevel = " << refinementLevel << std::endl;

            // minWidth is multiplied by refinement factor for the given number of
            // refinement levels
            for (int i = 1; i < numLevels; i++)
                minWidth *= refinementLevel;

            // if minWidth >= smallest original volume dimension * refinement factor
            // i.e. if the largest(?) brick dimension is a level greater than the
            // containing size of the volume
            // I suppose this is to work with non-brick-multiple volume dimensions
            if (minWidth >= refinementLevel * reduce_max(in->size()))
                throw std::runtime_error(
                        "too many levels, or too fine a refinement factor."
                        "do not have a single brick at the root...");
            // create a vec3 <minWidth, minWidth, minWidth>
            vec3i finestLevelSize = ospcommon::vec3i(minWidth);
            // and increment it until this dimension is contained
            while (finestLevelSize.x < in->size().x)
                finestLevelSize.x += minWidth;
            while (finestLevelSize.y < in->size().y)
                finestLevelSize.y += minWidth;
            while (finestLevelSize.z < in->size().z)
                finestLevelSize.z += minWidth;

            std::cout << "logical finest level size is " << finestLevelSize << std::endl;
            std::cout << "(note: input size was " << in->size() << ")" << std::endl;

            // at this point, finestLevelSize should be greater than or equal
            // to the input volume dimensions, so the name doesn't make much sense

            // create container for current level so we don't use in
            std::shared_ptr<Array3D<float>> currentLevel = in;

            /* create and write the bricks */
            vec3i levelSize = finestLevelSize;
            for (int level = numLevels - 1; level >= 0; --level) {
                std::cout << "-------------------------------------------------------"
                    << std::endl;
                std::cout << "logical size of level " << level << " is " << levelSize
                    << std::endl;
                // levelSize - currentLevelSize
                // nextLevelSize - next level (smaller than current) size
                const vec3i nextLevelSize = levelSize / refinementLevel;
                std::cout << "reducing to next level of " << nextLevelSize << std::endl;
                // create container for next level down
                std::shared_ptr<ActualArray3D<float>> nextLevel =
                    std::make_shared<ActualArray3D<float>>(nextLevelSize);
                // clear next level
                // i.e. initialize to zero
                for (int iz = 0; iz < nextLevelSize.z; iz++)
                    for (int iy = 0; iy < nextLevelSize.y; iy++)
                        for (int ix = 0; ix < nextLevelSize.x; ix++)
                            nextLevel->set(vec3i(ix, iy, iz), 0.f);

                // current level size divided by the brick size
                // blockSize was the starting/smallest(?) brick size, so
                // if blockSize == 1, numBricks == levelSize
                // so maybe input brick size shouldn't be 1 (????)
                const vec3i numBricks = levelSize / blockSize;
                PRINT(numBricks);
                PRINT(numBricks.product());
                // if this is supposed to update with progress on the command line,
                // then it doesn't work
                // Progress progress("level progress:", numBricks.product(), 30.f);
                // perform the lambda function for numBricks.product() indices
                ospcommon::tasking::parallel_for(numBricks.product(), [&](int brickIdx) {
                        // create new brick description
                        BrickDesc brick;
                        // set level to current level from outer for loop above
                        brick.level = level;
                        // dt == cellWidth in osp_amr_brick_info
                        brick.dt    = 1.f / powf(refinementLevel, level);
                        // get 3D brick index from flat brickIdx
                        const vec3i brickID(brickIdx % numBricks.x,
                                (brickIdx / numBricks.x) % numBricks.y,
                                brickIdx / (numBricks.x * numBricks.y));
                        // set upper and lower bounds of brick based on 3D index and
                        // brick size in input data space
                        brick.box.lower = brickID * blockSize;
                        brick.box.upper = brick.box.lower + (blockSize - 1);
                        // current brick data
                        std::vector<float> data(blockSize * blockSize * blockSize);
                        size_t out = 0;
                        ospcommon::range1f brickRange;
                        // traverse the data by brick index
                        for (int iz = brick.box.lower.z; iz <= brick.box.upper.z; iz++) {
                            for (int iy = brick.box.lower.y; iy <= brick.box.upper.y; iy++) {
                                for (int ix = brick.box.lower.x; ix <= brick.box.upper.x;
                                        ix++) {
                                    const vec3i thisLevelCoord(ix, iy, iz);
                                    const vec3i nextLevelCoord(ix / refinementLevel, iy / refinementLevel, iz / refinementLevel);
                                    // get the actual data at current coordinates
                                    const float v = currentLevel->get(thisLevelCoord);
                                    // insert the data into the current brick
                                    data[out++]   = v;
                                    // increment the data in the next level down by
                                    // the extracted value divided by the refinement level
                                    // (????)
                                    // this was initialized to zero above, so we're doing
                                    // some sort of accumulation here
                                    // at this point, I now think that nextLevel, though it has
                                    // a smaller extents, is actually the physically larger
                                    // brick, and contains a "low resolution" version of the
                                    // data in the current brick
                                    nextLevel->set(
                                            nextLevelCoord,
                                            nextLevel->get(nextLevelCoord) + (v / (refinementLevel * refinementLevel * refinementLevel)));
                                    // extend the value range of this brick (min/max) as needed
                                    brickRange.extend(v);
                                }
                            }
                        }

                        // write data to file if we're not removing(?) data
                        // here is where the threshold provided is coming into play
                        // if the current brick's data range is less than our threshold,
                        // we have removed data somehow. I guess because we don't end up
                        // writing any of it.
                        // if the range is greater than the threshold, we write it...
                        // (????)
                        std::lock_guard<std::mutex> lock(fileMutex);
                        if ((level > 0) &&
                                ((brickRange.upper - brickRange.lower) <= threshold)) {
                            numRemoved++;
                        } else {
                            brickInfo.push_back(brick);
                            brickData.push_back(data);
                            numWritten++;
                        }
                        //progress.ping();
                }); // end parallel for
                currentLevel = nextLevel;
                levelSize = nextLevelSize;
                std::cout << "done level " << level << ", written " << numWritten
                    << " bricks, removed " << numRemoved << std::endl;
            } // end for loop on levels
        }

        void outputAMR(const FileName outFileBase,
                       const std::vector<BrickDesc> &brickInfo,
                       const std::vector<std::vector<float>> &brickData,
                       const int blockSize)
        {
            // ALOK: .info is brick metadata
            FILE *infoOut = fopen(outFileBase.addExt(".info").c_str(), "wb");
            if (!infoOut) {
                throw std::runtime_error("could not open info output file!");
            }

            // ALOK: .data is actual data
            FILE *dataOut = fopen(outFileBase.addExt(".data").c_str(), "wb");
            if (!dataOut) {
                throw std::runtime_error("could not open data output file!");
            }

            std::ofstream osp(outFileBase + ".osp");
            osp << "<?xml?>" << std::endl;
            osp << "<AMRVolume>" << std::endl;
            osp << "  <fileName>" << ospcommon::FileName(outFileBase).base() << "</fileName>" << std::endl;
            osp << "  <brickSize>" << blockSize << "</brickSize>" << std::endl;
            osp << "  <clamp>0 100000</clamp>" << std::endl;
            osp << "</AMRVolume>" << std::endl;

            for(auto &brick : brickInfo) {
                fwrite(&brick, sizeof(brick), 1, infoOut);
            }

            for(auto &data : brickData) {
                fwrite(data.data(), sizeof(float), data.size(), dataOut);
            }

            fclose(infoOut);
            fclose(dataOut);
        }
    } // namespace amr
} // namespace ospray

#pragma clang diagnostic pop
