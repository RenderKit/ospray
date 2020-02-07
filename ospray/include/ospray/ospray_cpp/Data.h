// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
#include "Traits.h"
// stl
#include <array>
#include <vector>

namespace ospray {
namespace cpp {

class Data : public ManagedObject<OSPData, OSP_DATA>
{
 public:
  // Construction from an existing array (infer element type)

  template <typename T>
  Data(size_t numItems, const T *init, bool isShared = false);

  template <typename T>
  Data(
      size_t numItems, size_t byteStride, const T *init, bool isShared = false);

  template <typename T>
  Data(const vec2ul &numItems, const T *init, bool isShared = false);

  template <typename T>
  Data(const vec2ul &numItems,
      const vec2ul &byteStride,
      const T *init,
      bool isShared = false);

  template <typename T>
  Data(const vec3ul &numItems, const T *init, bool isShared = false);

  template <typename T>
  Data(const vec3ul &numItems,
      const vec3ul &byteStride,
      const T *init,
      bool isShared = false);

  // Adapters to work with existing STL containers

  template <typename T, std::size_t N>
  Data(const std::array<T, N> &arr, bool isShared = false);

  template <typename T, typename ALLOC_T>
  Data(const std::vector<T, ALLOC_T> &arr, bool isShared = false);

  // Set a single object as a 1-item data array

  template <typename T>
  Data(const T &obj);

  // Construct from an existing OSPData handle (cpp::Data then owns handle)

  Data(OSPData existing = nullptr);

 private:
  template <typename T>
  void validate_element_type();
};

static_assert(
    sizeof(Data) == sizeof(OSPData), "cpp::Data can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

template <typename T>
inline Data::Data(size_t numItems, const T *init, bool isShared)
    : Data(numItems, 0, init, isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(
    size_t numItems, size_t byteStride, const T *init, bool isShared)
    : Data(vec3ul(numItems, 1, 1), vec3ul(byteStride, 0, 0), init, isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(const vec2ul &numItems, const T *init, bool isShared)
    : Data(numItems, vec2ul(0), init, isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(const vec2ul &numItems,
    const vec2ul &byteStride,
    const T *init,
    bool isShared)
    : Data(vec3ul(numItems.x, numItems.y, 1),
        vec3ul(byteStride.x, byteStride.y, 0),
        init,
        isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(const vec3ul &numItems, const T *init, bool isShared)
    : Data(numItems, vec3ul(0), init, isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(const vec3ul &numItems,
    const vec3ul &byteStride,
    const T *init,
    bool isShared)
{
  validate_element_type<T>();

  auto format = OSPTypeFor<T>::value;

  auto tmp = ospNewSharedData(init,
      format,
      numItems.x,
      byteStride.x,
      numItems.y,
      byteStride.y,
      numItems.z,
      byteStride.z);

  if (isShared) {
    ospObject = tmp;
  } else {
    ospObject = ospNewData(format, numItems.x, numItems.y, numItems.z);
    ospCopyData(tmp, ospObject);
    ospRelease(tmp);
  }
}

template <typename T, std::size_t N>
inline Data::Data(const std::array<T, N> &arr, bool isShared)
    : Data(N, arr.data(), isShared)
{
  validate_element_type<T>();
}

template <typename T, typename ALLOC_T>
inline Data::Data(const std::vector<T, ALLOC_T> &arr, bool isShared)
    : Data(arr.size(), arr.data(), isShared)
{
  validate_element_type<T>();
}

template <typename T>
inline Data::Data(const T &obj) : Data(1, &obj)
{
  validate_element_type<T>();
}

inline Data::Data(OSPData existing) : ManagedObject<OSPData, OSP_DATA>(existing)
{}

template <typename T>
inline void Data::validate_element_type()
{
  static_assert(OSPTypeFor<T>::value != OSP_UNKNOWN,
      "Only types corresponding to OSPDataType values can be set "
      "as elements in OSPRay Data arrays. NOTE: Math types (vec, "
      "box, linear, affine) are expected to come from ospcommon::math.");
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Data, OSP_DATA);

} // namespace ospray
