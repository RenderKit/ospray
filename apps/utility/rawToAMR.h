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

// pragmas?

#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
#include "ospcommon/FileName.h"
#include "ospcommon/tasking/parallel_for.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// ALOK: TODO
// remove these if possible
using namespace ospcommon::array3D;
using namespace ospcommon;

namespace ospray {
    // ALOK: TODO
    // move these out of the namespace if possible
    // (here because both amr and vtu had these)
    static size_t numWritten = 0;
    static size_t numRemoved = 0;

    namespace amr {
        // ALOK: TODO
        // remove this and use osp_amr_brick_info instead
        struct BrickDesc
        {
            box3i box;
            int level;
            float dt;
        };

        // ALOK: TODO
        // move these out of the namespace if possible
        static std::mutex fileMutex;
        //static FILE *infoOut = nullptr;
        //static FILE *dataOut = nullptr;

        void makeAMR(const std::shared_ptr<Array3D<float>> in,
                     const int numLevels,
                     const int blockSize,
                     const int refinementLevel,
                     const float threshold,
                     std::vector<BrickDesc> &brickInfo,
                     std::vector<std::vector<float>> &brickData);

        void outputAMR(const FileName outFileBase,
                       const std::vector<BrickDesc> &brickInfo,
                       const std::vector<std::vector<float>> &brickData,
                       const int blockSize);
    } // namespace amr
} // namespace ospray
