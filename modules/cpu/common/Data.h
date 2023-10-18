// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iterator>
#include <memory>
#include "ISPCDeviceObject.h"
#include "StructShared.h"
#include "ispcrt.hpp"

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
struct OSPRAY_SDK_INTERFACE Data : public ISPCDeviceObject
{
  Data(api::ISPCDevice &device,
      const void *sharedData,
      OSPDataType,
      const vec3ul &numItems,
      const vec3l &byteStride,
      OSPDeleterCallback freeFunction = nullptr,
      const void *userData = nullptr);
  Data(api::ISPCDevice &device, OSPDataType, const vec3ul &numItems);

  virtual ~Data() override;
  virtual std::string toString() const override;

  size_t size() const;
  vec3l stride() const;
  char *data() const;
  char *data(const vec3ul &idx) const;
  bool compact() const; // all strides are natural
  void copy(const Data &source, const vec3ul &destinationIndex);

#ifdef OSPRAY_TARGET_SYCL
  void commit() override;
#endif

  bool isShared() const;

  template <typename T, int DIM = 1>
  const DataT<T, DIM> &as() const;

  template <typename T, int DIM = 1>
  DataT<T, DIM> &as();

  template <typename T, int DIM>
  typename std::enable_if<std::is_pointer<T>::value, bool>::type is() const;

  template <typename T, int DIM>
  typename std::enable_if<!std::is_pointer<T>::value, bool>::type is() const;

 protected:
  // The actual buffer storing the data
  std::unique_ptr<BufferShared<char>> view;
  char *addr{nullptr};
  // We need to track the appSharedPtr separately for the GPU backend, if we
  // were passed a shared ptr to memory that was not in USM we made a copy that
  // addr points to and we need to keep it in sync with the shared data from the
  // app.
  char *appSharedPtr{nullptr};
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

 protected:
  OSPDeleterCallback freeFunction{nullptr};
  const void *userData{nullptr};
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
  using iterator = Iter<T>;

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
  using iterator = Iter1D<T>;

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

template <typename T>
struct OSPTypeFor<DataT<T> *>
{
  static constexpr OSPDataType value = OSP_DATA;
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

template <typename T>
ispc::Data1D *ispc(Ref<DataT<T, 1>> &dataRef)
{
  return dataRef ? &dataRef->ispc : &Data::emptyData1D;
}

inline size_t Data::size() const
{
  return numItems.x * numItems.y * numItems.z;
}

inline vec3l Data::stride() const
{
  return byteStride;
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
  auto toType = OSPTypeFor<typename std::remove_const<
      typename std::remove_pointer<T>::type>::type *>::value;
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
inline DataT<T, DIM> &Data::as()
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
inline const Ref<const DataT<T, DIM>> ISPCDeviceObject::getParamDataT(
    const char *name, bool required, bool promoteScalar)
{
  Data *data = getParamObject<Data>(name);

  if (data && data->is<T, DIM>())
    return &(data->as<T, DIM>());

  // if no data array is found, look for single item of same type
  if (promoteScalar) {
    auto item = getOptParam<T>(name);
    if (item) {
      // create data array and its reference object
      data = new Data(getISPCDevice(), OSPTypeFor<T>::value, vec3ul(1));
      Ref<const DataT<T, DIM>> refDataT = &data->as<T, DIM>();

      // 'data' reference counter equals to 2 now,
      // but we want it to be 1 and the 'data' to be referenced only by the
      // returned object for proper destruction
      data->refDec();

      // place value into 'data' array
      T *p = refDataT->data();
      *p = item.value();

      // if we are inserting 'ManagedObject' we need to increase its reference
      // counter, too
      if (isObjectType(data->type))
        (*data->as<ManagedObject *, DIM>().begin())->refInc();
      return refDataT;
    }
  }

  if (required) {
    std::string msg(toString() + " must have '" + name + "' "
        + std::to_string(DIM) + "D array with element type "
        + stringFor(OSPTypeFor<T>::value));
    if (data)
      msg.append(", found " + std::to_string(data->dimensions)
          + "D array with element type " + stringFor(data->type));
    throw std::runtime_error(msg);
  } else {
    if (data) {
      postStatusMsg(OSP_LOG_DEBUG)
          << toString() << " ignoring '" << name << "' " << data->dimensions
          << "D array with element type " << stringFor(data->type)
          << " (while looking for " << DIM << "D array of "
          << stringFor(OSPTypeFor<T>::value) << ")";
    }
    return nullptr;
  }
}

template <typename T, typename U, int DIM>
std::vector<T *> createArrayOfSh(const DataT<U *, DIM> &data)
{
  std::vector<T *> retval;
  retval.reserve(data.size());

  for (auto &&obj : data)
    retval.push_back(obj->getSh());

  return retval;
}

} // namespace ospray
