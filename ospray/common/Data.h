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

#pragma once

// ospray
#include "./Managed.h"

namespace ospray {

  /*! \brief defines a data array (aka "buffer") type that contains
      'n' items of a given type */
  struct OSPRAY_SDK_INTERFACE Data : public ManagedObject
  {
    Data(const void *sharedData,
        OSPDataType,
        const vec3ui &numItems,
        const vec3l &byteStride);
    Data(OSPDataType, const vec3ui &numItems);

    virtual ~Data() override;
    virtual std::string toString() const override;

    size_t size() const;
    char *data() const;
    char *data(const vec3ui &idx) const;
    void copy(const Data &source, const vec3ui &destinationIndex);

    /*! Iterator begin/end for Data arrays */
    template <typename T>
    T *begin();

    template <typename T>
    T *end();

    template <typename T>
    const T *begin() const;

    template <typename T>
    const T *end() const;

    template <typename T>
    T &at(size_t i);

    template <typename T>
    const T &at(size_t i) const;

    size_t numBytes; // XXX temp

   protected:
    char *addr{nullptr};
    bool shared;
   public:
    OSPDataType type{OSP_UNKNOWN};
   protected:
    vec3ui numItems;
    vec3l byteStride;
    int dimensions{0};

   private:
    void init(); // init dimensions and byteStride
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline size_t Data::size() const
  {
    return numItems.x * size_t(numItems.y) * numItems.z;
  }

  inline char *Data::data() const
  {
    return addr;
  }

  inline char *Data::data(const vec3ui &idx) const
  {
    return addr + idx.x * byteStride.x + idx.y * byteStride.y
        + idx.z * byteStride.z;
  }

  template <typename T>
  inline T *Data::begin()
  {
    return (T *)(addr);
  }

  template <typename T>
  inline T *Data::end()
  {
    return begin<T>() + numItems.x;
  }

  template <typename T>
  inline const T *Data::begin() const
  {
    return static_cast<const T *>(data)();
  }

  template <typename T>
  inline const T *Data::end() const
  {
    return begin<const T>() + numItems.x;
  }

  template <typename T>
  inline T &Data::at(size_t i)
  {
    return *(begin<T>() + i);
  }

  template <typename T>
  inline const T &Data::at(size_t i) const
  {
    return *(begin<T>() + i);
  }

  // Helper functions /////////////////////////////////////////////////////////

  inline std::vector<void *> createArrayOfIE(Data &data)
  {
    if (!isObjectType(data.type))
      throw std::runtime_error("cannot createArrayOfIE() with non OSP_OBJECT!");

    std::vector<void *> retval;
    retval.resize(data.size());

    auto begin = data.begin<ManagedObject *>();
    auto end   = data.end<ManagedObject *>();
    std::transform(begin, end, retval.begin(), [](ManagedObject *obj) {
      return obj->getIE();
    });

    return retval;
  }

}  // namespace ospray
