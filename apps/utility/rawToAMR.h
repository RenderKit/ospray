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

#include "ospcommon/FileName.h"
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"

#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <fcntl.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ospray {
  namespace amr {
    using namespace ospcommon;

    void makeAMR(const std::vector<float> &in,
                 const vec3i inGridDims,
                 const int numLevels,
                 const int blockSize,
                 const int refinementLevel,
                 const float threshold,
                 std::vector<box3i> &blockBounds,
                 std::vector<int> &refinementLevels,
                 std::vector<float> &cellWidths,
                 std::vector<std::vector<float>> &brickData);

    void outputAMR(const FileName outFileBase,
                   const std::vector<box3i> &blockBounds,
                   const std::vector<int> &refinementLevels,
                   const std::vector<float> &cellWidths,
                   const std::vector<std::vector<float>> &brickData,
                   const int blockSize);

    template <typename T>
    std::vector<T> loadRAW(const std::string &fileName, const vec3i &dims);

    template <typename T>
    std::vector<T> mmapRAW(const std::string &fileName, const vec3i &dims)
    {
#ifdef _WIN32
      throw std::runtime_error("Memory mapping not supported in Windows");
#else
      FILE *file = fopen(fileName.c_str(), "rb");
      fseek(file, 0, SEEK_END);
      size_t actualFileSize = ftell(file);
      fclose(file);

      size_t fileSize =
          size_t(dims.x) * size_t(dims.y) * size_t(dims.z) * sizeof(T);

      if (actualFileSize != fileSize) {
        std::stringstream ss;
        ss << "Got file size " << prettyNumber(actualFileSize);
        ss << ", but expected " << prettyNumber(fileSize);
        ss << ". Common cause is wrong data type!";
        throw std::runtime_error(ss.str());
      }

      int fd = ::open(fileName.c_str(), O_RDONLY);
      assert(fd >= 0);

      void *mem = mmap(nullptr,
                       fileSize,
                       PROT_READ,
                       MAP_SHARED  // |MAP_HUGETLB
                       ,
                       fd,
                       0);
      assert(mem != nullptr && (long long)mem != -1LL);

      std::vector<T> volume;
      volume.assign((T *)mem, (T *)mem + dims.product());

      return volume;
#endif
    }
  }  // namespace amr
}  // namespace ospray
