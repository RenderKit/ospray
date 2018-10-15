// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// ospray
#include "Array3D.h"

// stdlib, for mmap
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <cstring>
#include <string>

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define O_LARGEFILE 0
#endif

namespace ospcommon {
  namespace array3D {

    template <typename T>
    std::shared_ptr<Array3D<T>> loadRAW(const std::string &fileName,
                                        const vec3i &dims)
    {
      std::shared_ptr<ActualArray3D<T>> volume =
          std::make_shared<ActualArray3D<T>>(dims);
      FILE *file = fopen(fileName.c_str(), "rb");
      if (!file)
        throw std::runtime_error("ospray::amr::loadRaw(): could not open '" +
                                 fileName + "'");
      const size_t num = size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
      size_t numRead   = fread(volume->value, sizeof(T), num, file);
      if (num != numRead)
        throw std::runtime_error(
            "ospray::amr::loadRaw(): read incomplete data ...");
      fclose(file);

      return std::move(volume);
    }

    template <typename T>
    std::shared_ptr<Array3D<T>> mmapRAW(const std::string &fileName,
                                        const vec3i &dims)
    {
#ifdef _WIN32
      throw std::runtime_error("mmap not supported under windows");
#else
      FILE *file = fopen(fileName.c_str(), "rb");
      fseek(file, 0, SEEK_END);
      size_t actualFileSize = ftell(file);
      PRINT(actualFileSize);
      fclose(file);

      size_t fileSize =
          size_t(dims.x) * size_t(dims.y) * size_t(dims.z) * sizeof(T);
      std::cout << "mapping file " << fileName << " exptd size "
                << prettyNumber(fileSize) << " actual size "
                << prettyNumber(actualFileSize) << std::endl;
      if (actualFileSize < fileSize)
        throw std::runtime_error("incomplete file!");
      if (actualFileSize > fileSize)
        throw std::runtime_error("mapping PARTIAL (or incorrect!?) file...");
      int fd = ::open(fileName.c_str(), O_LARGEFILE | O_RDONLY);
      assert(fd >= 0);

      void *mem = mmap(nullptr,
                       fileSize,
                       PROT_READ,
                       MAP_SHARED  // |MAP_HUGETLB
                       ,
                       fd,
                       0);
      assert(mem != nullptr && (long long)mem != -1LL);

      std::shared_ptr<ActualArray3D<T>> volume =
          std::make_shared<ActualArray3D<T>>(dims, mem);

      return std::move(volume);
#endif
    }


    // -------------------------------------------------------
    // explicit instantiations section
    // -------------------------------------------------------

    template struct Array3D<uint8_t>;
    template struct Array3D<float>;
    template struct Array3D<double>;
    template struct ActualArray3D<uint8_t>;
    template struct ActualArray3D<float>;
    template struct ActualArray3D<double>;

    template struct Array3DAccessor<uint8_t, uint8_t>;
    template struct Array3DAccessor<float, uint8_t>;
    template struct Array3DAccessor<double, uint8_t>;
    template struct Array3DAccessor<uint8_t, float>;
    template struct Array3DAccessor<float, float>;
    template struct Array3DAccessor<double, float>;
    template struct Array3DAccessor<uint8_t, double>;
    template struct Array3DAccessor<float, double>;
    template struct Array3DAccessor<double, double>;

    template struct Array3DRepeater<uint8_t>;
    template struct Array3DRepeater<float>;

    template std::shared_ptr<Array3D<uint8_t>> loadRAW(
        const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<float>> loadRAW(
        const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<double>> loadRAW(
        const std::string &fileName, const vec3i &dims);

    template std::shared_ptr<Array3D<uint8_t>> mmapRAW(
        const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<float>> mmapRAW(
        const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<double>> mmapRAW(
        const std::string &fileName, const vec3i &dims);

  }  // ::ospcommon::array3D
}  // ::ospcommon
