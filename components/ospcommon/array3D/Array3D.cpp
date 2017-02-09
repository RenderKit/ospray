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

// ospray
#include "Array3D.h"
#include "for_each.h"

// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif
#include <fcntl.h>
#include <string>
#include <cstring>

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

namespace ospcommon {
  namespace array3D {

    // Array3D //

    /*! get the range/interval of all cell values in the given
      begin/end region of the volume */
    template<typename T>
    Range<T> Array3D<T>::getValueRange(const vec3i &begin,
                                       const vec3i &end) const
    {
      Range<T> v = get(begin);
      for_each(begin,end,[&](const vec3i &idx){
          v.extend(get(idx));
        });
      return v;
    }

    template<typename T>
    std::shared_ptr<Array3D<T>> loadRAW(const std::string &fileName, const vec3i &dims)
    {
      std::shared_ptr<ActualArray3D<T>> volume = std::make_shared<ActualArray3D<T>>(dims);
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file)
        throw std::runtime_error("ospray::amr::loadRaw(): could not open '"+fileName+"'");
      const size_t num = size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
      size_t numRead = fread(volume->value,sizeof(T),num,file);
      if (num != numRead)
        throw std::runtime_error("ospray::amr::loadRaw(): read incomplete data ...");
      fclose(file);

      return volume;
    }
    
    template<typename T>
    std::shared_ptr<Array3D<T>> mmapRAW(const std::string &fileName, const vec3i &dims)
    {
#ifdef _WIN32
      throw std::runtime_error("mmap not supported under windows");
#else
      FILE *file = fopen(fileName.c_str(),"rb");
      fseek(file,0,SEEK_END);
      size_t actualFileSize = ftell(file);
      PRINT(actualFileSize);
      fclose(file);

      size_t fileSize = size_t(dims.x)*size_t(dims.y)*size_t(dims.z)*sizeof(T);
      std::cout << "mapping file " << fileName
                << " exptd size " << prettyNumber(fileSize) << " actual size " << prettyNumber(actualFileSize) << std::endl;
      if (actualFileSize < fileSize)
        throw std::runtime_error("incomplete file!");
      if (actualFileSize > fileSize)
        throw std::runtime_error("mapping PARTIAL (or incorrect!?) file...");
      int fd = ::open(fileName.c_str(), O_LARGEFILE | O_RDONLY);
      assert(fd >= 0);

      void *mem = mmap(nullptr,fileSize,PROT_READ,MAP_SHARED// |MAP_HUGETLB
                       ,fd,0);
      assert(mem != nullptr && (long long)mem != -1LL);

      std::shared_ptr<ActualArray3D<T>> volume = std::make_shared<ActualArray3D<T>>(dims,mem);

      return volume;
#endif
    }
    
    // ActualArray3D //

    template<typename T>
    size_t ActualArray3D<T>::indexOf(const vec3i &pos) const
    {
      return pos.x+size_t(dims.x)*(pos.y+size_t(dims.y)*pos.z);
    }

    // Array3DAccessor //

    template<typename in_t, typename out_t>
    Array3DAccessor<in_t, out_t>::Array3DAccessor(std::shared_ptr<Array3D<in_t>> actual) :
      actual(actual)
    {}
    
    template<typename in_t, typename out_t>
    vec3i Array3DAccessor<in_t, out_t>::size() const
    {
      return actual->size();
    }
    
    template<typename in_t, typename out_t>
    out_t Array3DAccessor<in_t, out_t>::get(const vec3i &where) const
    {
      return (out_t)actual->get(where);
    }

    template<typename in_t, typename out_t>
    size_t Array3DAccessor<in_t, out_t>::numElements() const 
    { 
      assert(actual); return actual->numElements(); 
    }

    // Array3DRepeater //

    template<typename T>
    Array3DRepeater<T>::Array3DRepeater(const std::shared_ptr<Array3D<T>> &actual, 
                                        const vec3i &repeatedSize) :
      actual(actual), repeatedSize(repeatedSize)
    {}
    
    template<typename T>
    vec3i Array3DRepeater<T>::size() const
    {
      return repeatedSize;
    }
    
    template<typename T>
    T Array3DRepeater<T>::get(const vec3i &_where) const
    {
      vec3i where(_where.x % repeatedSize.x,
                  _where.y % repeatedSize.y,
                  _where.z % repeatedSize.z);

      if ((_where.x / repeatedSize.x) % 2) where.x = repeatedSize.x - 1 - where.x;
      if ((_where.y / repeatedSize.y) % 2) where.y = repeatedSize.y - 1 - where.y;
      if ((_where.z / repeatedSize.z) % 2) where.z = repeatedSize.z - 1 - where.z;

      return actual->get(where);
    }

    template<typename T>
    size_t Array3DRepeater<T>::numElements() const 
    { 
      return size_t(repeatedSize.x)*size_t(repeatedSize.y)*size_t(repeatedSize.z);
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

    template struct Array3DAccessor<uint8_t,uint8_t>;
    template struct Array3DAccessor<float,uint8_t>;
    template struct Array3DAccessor<double,uint8_t>;
    template struct Array3DAccessor<uint8_t,float>;
    template struct Array3DAccessor<float,float>;
    template struct Array3DAccessor<double,float>;
    template struct Array3DAccessor<uint8_t,double>;
    template struct Array3DAccessor<float,double>;
    template struct Array3DAccessor<double,double>;

    template struct Array3DRepeater<uint8_t>;
    template struct Array3DRepeater<float>;

    template std::shared_ptr<Array3D<uint8_t>> loadRAW(const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<float>>   loadRAW(const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<double>>  loadRAW(const std::string &fileName, const vec3i &dims);

    template std::shared_ptr<Array3D<uint8_t>> mmapRAW(const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<float>>   mmapRAW(const std::string &fileName, const vec3i &dims);
    template std::shared_ptr<Array3D<double>>  mmapRAW(const std::string &fileName, const vec3i &dims);

  } // ::ospcommon::array3D
} // ::ospcommon
