// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iterator>
#include "./Managed.h"

// including "Data_ispc.h" breaks app code using SDK headers
#ifndef __ISPC_STRUCT_Data1D__
#define __ISPC_STRUCT_Data1D__
namespace ispc {
struct Data1D
{
  uint8_t *addr;
  int64_t byteStride;
  uint32_t numItems;
  bool huge;
};
} // namespace ispc
#endif

namespace ospray {

template <typename T, int DIM = 1>
struct DataT;

/*! \brief defines a data array (aka "buffer") type that contains
    'n' items of a given type */
struct OSPRAY_SDK_INTERFACE Data : public ManagedObject
{
  Data(const void *sharedData,
      OSPDataType,
      const vec3ul &numItems,
      const vec3l &byteStride);
  Data(OSPDataType, const vec3ul &numItems);

  virtual ~Data() override;
  virtual std::string toString() const override;

  size_t size() const;
  char *data() const;
  char *data(const vec3ul &idx) const;
  bool compact() const; // all strides are natural
  void copy(const Data &source, const vec3ul &destinationIndex);

  template <typename T, int DIM = 1>
  const DataT<T, DIM> &as() const;

  template <typename T, int DIM>
  typename std::enable_if<std::is_pointer<T>::value, bool>::type is() const;

  template <typename T, int DIM>
  typename std::enable_if<!std::is_pointer<T>::value, bool>::type is() const;

 protected:
  char *addr{nullptr};
  bool shared;

 public:
  OSPDataType type{OSP_UNKNOWN};
  vec3ul numItems;

 protected:
  vec3l byteStride;

 public:
  int dimensions{0};

  ispc::Data1D ispc;
  static ispc::Data1D emptyData1D; // dummy, zero-initialized

 private:
  void init(); // init dimensions and byteStride
};

OSPTYPEFOR_SPECIALIZATION(Data *, OSP_DATA);

template <typename T>
class Iter : public std::iterator<std::forward_iterator_tag, T>
{
  const Data &data;
  vec3ul idx;

 public:
  Iter(const Data &data, vec3ul idx) : data(data), idx(idx) {}
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
  Iter1D(char *addr, int64_t byteStride) : addr(addr), byteStride(byteStride) {}
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
  bool operator==(const Iter1D &other) const
  {
    return addr == other.addr;
  }
  bool operator!=(const Iter1D &other) const
  {
    return addr != other.addr;
  }
  T &operator*() const
  {
    return *reinterpret_cast<T *>(addr);
  }
  T *operator->() const
  {
    return reinterpret_cast<T *>(addr);
  }
};

template <typename T, int DIM>
struct DataT : public Data
{
  static_assert(DIM == 2 || DIM == 3, "only 1D, 2D or 3D DataT supported");
  using value_type = T;
  using interator = Iter<T>;

  Iter<T> begin() const
  {
    return Iter<T>(*this, vec3ul(0));
  }
  Iter<T> end() const
  {
    return Iter<T>(*this, vec3ul(0, 0, numItems.z));
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
struct DataT<T, 1> : public Data
{
  using value_type = T;
  using interator = Iter1D<T>;

  Iter1D<T> begin() const
  {
    return Iter1D<T>(addr, ispc.byteStride);
  }
  Iter1D<T> end() const
  {
    return Iter1D<T>(addr + ispc.byteStride * size(), ispc.byteStride);
  }

  T &operator[](int64_t idx)
  {
    return *reinterpret_cast<T *>(addr + ispc.byteStride * idx);
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
    return ispc.byteStride;
  }
};

// Inlined definitions //////////////////////////////////////////////////////

inline const ispc::Data1D *ispc(Ref<const Data> &dataRef)
{
  if (dataRef && dataRef->size() > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error(
        "data array too large (over 4B elements, index is limited to 32bit");

  return dataRef && dataRef->dimensions == 1 ? &dataRef->ispc
                                             : &Data::emptyData1D;
}

template <typename T>
const ispc::Data1D *ispc(Ref<const DataT<T, 1>> &dataRef)
{
  return dataRef ? &dataRef->ispc : &Data::emptyData1D;
}

inline size_t Data::size() const
{
  return numItems.x * numItems.y * numItems.z;
}

inline char *Data::data() const
{
  return addr;
}

inline char *Data::data(const vec3ul &idx) const
{
  return addr + idx.x * byteStride.x + idx.y * byteStride.y
      + idx.z * byteStride.z;
}

template <typename T, int DIM>
inline typename std::enable_if<std::is_pointer<T>::value, bool>::type Data::is()
    const
{
  auto toType = OSPTypeFor<T>::value;
  return (type == toType || (toType == OSP_OBJECT && isObjectType(type)))
      && dimensions <= DIM; // can iterate with higher dimensionality
}

template <typename T, int DIM>
inline typename std::enable_if<!std::is_pointer<T>::value, bool>::type
Data::is() const
{
  // can iterate with higher dimensionality
  return type == OSPTypeFor<T>::value && dimensions <= DIM;
}

template <typename T, int DIM>
inline const DataT<T, DIM> &Data::as() const
{
  if (is<T, DIM>())
    return (DataT<T, DIM> &)*this;
  else {
    std::stringstream ss;
    ss << "Incompatible type or dimension for DataT; requested type[dim]: "
       << stringFor(OSPTypeFor<T>::value) << "[" << DIM
       << "], actual: " << stringFor(type) << "[" << dimensions << "].";
    throw std::runtime_error(ss.str());
  }
}

template <typename T, int DIM>
inline const Ref<const DataT<T, DIM>> ManagedObject::getParamDataT(
    const char *name, bool required, bool promoteScalar)
{
  Data *data = getParam<Data *>(name);

  if (data && data->is<T, DIM>())
    return &(data->as<T, DIM>());

  // if no data array is found, look for single item of same type
  if (promoteScalar) {
    auto item = getOptParam<T>(name);
    if (item) {
      // wrap item into data array
      data = new Data(OSPTypeFor<T>::value, vec3ul(1));
      auto &dataT = data->as<T, DIM>();
      T *p = dataT.data();
      *p = item.value();
      return &dataT;
    }
  }

  if (required)
    throw std::runtime_error(toString() + " must have '" + name
        + "' array with element type " + stringFor(OSPTypeFor<T>::value));
  else {
    if (data) {
      postStatusMsg(OSP_LOG_DEBUG)
          << toString() << " ignoring '" << name
          << "' array with wrong element type (should be "
          << stringFor(OSPTypeFor<T>::value) << ")";
    }
    return nullptr;
  }
}

template <typename T, int DIM>
inline typename std::enable_if<std::is_base_of<ManagedObject, T>::value,
    std::vector<void *>>::type
createArrayOfIE(const DataT<T *, DIM> &data)
{
  std::vector<void *> retval;

  for (auto &&obj : data)
    retval.push_back(obj->getIE());

  return retval;
}
} // namespace ospray
