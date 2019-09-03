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

#include <iterator>
#include "./Managed.h"

namespace ospray {

  template <typename T, int DIM = 1>
  struct OSPRAY_SDK_INTERFACE DataT;

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

    template <typename T, int DIM = 1>
    typename std::enable_if<std::is_pointer<T>::value, DataT<T, DIM> &>::type
    as() const;
    template <typename T, int DIM = 1>
    typename std::enable_if<!std::is_pointer<T>::value, DataT<T, DIM> &>::type
    as() const;

    size_t numBytes; // XXX temp

   protected:
    char *addr{nullptr};
    bool shared;

   public:
    OSPDataType type{OSP_UNKNOWN};
    vec3ui numItems;

   protected:
    vec3l byteStride;
    int dimensions{0};

   private:
    void init(); // init dimensions and byteStride

    template <typename T, int DIM>
    DataT<T, DIM> &checkType(OSPDataType toType, bool fallback = false) const;
  };

  std::vector<void *> createArrayOfIE(Data &data);

  template <typename T>
  class Iter : public std::iterator<std::forward_iterator_tag, T>
  {
    const Data &data;
    vec3ui idx;

   public:
    Iter(const Data &data, vec3ui idx) : data(data), idx(idx) {}
    Iter &operator++()
    {
      if (++idx.x >= data.numItems.x) {
        idx.x = 0;
        if (++idx.y >= data.numItems.y) {
          idx.y = 0;
          ++idx.z;
        }
      }
      return *this;
    }
    Iter operator++(int)
    {
      Iter retv(*this);
      ++(*this);
      return retv;
    }
    bool operator!=(Iter &other) const
    {
      return &data != &other.data || idx != other.idx;
    }
    T &operator*() const
    {
      return *reinterpret_cast<T *>(data.data(idx));
    }
  };

  template <typename T>
  class Iter1D : public std::iterator<std::forward_iterator_tag, T>
  {
    char *addr{nullptr};
    int64_t byteStride{1};

   public:
    Iter1D(char *addr, int64_t byteStride) : addr(addr), byteStride(byteStride)
    {}
    Iter1D &operator++()
    {
      addr += byteStride;
      return *this;
    }
    Iter1D operator++(int)
    {
      Iter1D retv(*this);
      ++(*this);
      return retv;
    }
    bool operator==(Iter1D &other) const
    {
      return addr == other.addr;
    }
    bool operator!=(Iter1D &other) const
    {
      return addr != other.addr;
    }
    T &operator*() const
    {
      return *reinterpret_cast<T *>(addr);
    }
  };

  template <typename T, int DIM>
  struct OSPRAY_SDK_INTERFACE DataT : public Data
  {
    static_assert(DIM == 2 || DIM == 3, "only 1D, 2D or 3D DataT supported");
    using value_type = T;
    using interator = Iter<T>;

    Iter<T> begin() const
    {
      return Iter<T>(*this, vec3ui(0));
    }
    Iter<T> end() const
    {
      return Iter<T>(*this, vec3ui(0, 0, numItems.z));
    }

    T &operator[](const vec_t<int64_t, DIM> &idx)
    {
      return *reinterpret_cast<T *>(data(idx));
    }
    const T &operator[](const vec_t<int64_t, DIM> &idx) const
    {
      return const_cast<DataT<T, DIM> *>(this)->operator[](idx);
    }

    T *data() const
    {
      return reinterpret_cast<T *>(addr);
    }
  };

  template <typename T>
  struct OSPRAY_SDK_INTERFACE DataT<T, 1> : public Data
  {
    using value_type = T;
    using interator = Iter1D<T>;

    Iter1D<T> begin() const
    {
      return Iter1D<T>(addr, byteStride.x);
    }
    Iter1D<T> end() const
    {
      return Iter1D<T>(addr + byteStride.x * numItems.x, byteStride.x);
    }

    T &operator[](int64_t idx)
    {
      return *reinterpret_cast<T *>(addr + byteStride.x * idx);
    }
    const T &operator[](int64_t idx) const
    {
      return const_cast<DataT<T, 1> *>(this)->operator[](idx);
    }

    T *data() const
    {
      return reinterpret_cast<T *>(addr);
    }

    int64_t stride() const
    {
      return byteStride.x;
    }
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

  template <typename T, int DIM>
  DataT<T, DIM> &Data::checkType(OSPDataType toType, bool fallback) const
  {
    // can iterate with higher dimensionality
    if ((type == toType || fallback) && dimensions <= DIM)
      return (DataT<T, DIM> &)*this;
    else {
      std::stringstream ss;
      ss << "Incompatible type or dimension for DataT; requested type[dim]: "
         << stringForType(toType) << "[" << DIM
         << "], actual: " << stringForType(type) << "[" << dimensions << "].";
      throw std::runtime_error(ss.str());
    }
  }

  template <typename T, int DIM>
  typename std::enable_if<std::is_pointer<T>::value, DataT<T, DIM> &>::type
  Data::as() const
  {
    // XXX need OSPTypeFor for ospray::Objects
    //auto toType = OSPTypeFor<typename std::remove_pointer<T>::type>::value;
    auto toType = OSP_OBJECT;
    return checkType<T, DIM>(
        toType, toType == OSP_OBJECT && isObjectType(type));
  }

  template <typename T, int DIM>
  typename std::enable_if<!std::is_pointer<T>::value, DataT<T, DIM> &>::type
  Data::as() const
  {
    return checkType<T, DIM>(OSPTypeFor<T>::value);
  }

}  // namespace ospray
