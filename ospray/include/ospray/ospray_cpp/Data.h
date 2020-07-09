// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
#include "Traits.h"
// stl
#include <array>
#include <vector>

namespace ospray {
namespace cpp {

template <bool SHARED = false>
class Data : public ManagedObject<OSPData, OSP_DATA>
{
 public:
  // Construction from an existing array (explicit element type and size/stride)

  template <typename DIM_T>
  Data(const void *init, OSPDataType type, const DIM_T &numItems);

  template <typename DIM_T>
  Data(const void *init,
      OSPDataType type,
      const DIM_T &numItems,
      const DIM_T &byteStride);

  // Construction from an existing array (infer element type and size/stride)

  template <typename T, typename DIM_T>
  Data(const T *init, const DIM_T &numItems);

  template <typename T, typename DIM_T>
  Data(const T *init, const DIM_T &numItems, const DIM_T &byteStride);

  // Adapters to work with existing STL containers

  template <typename T, std::size_t N>
  Data(const std::array<T, N> &arr);

  template <typename T, typename ALLOC_T>
  Data(const std::vector<T, ALLOC_T> &arr);

  // Set a single object as a 1-item data array

  template <typename T>
  Data(const T &obj);

  // Construct from an existing OSPData handle (cpp::Data then owns handle)

  Data(OSPData existing = nullptr);

 private:
  template <typename T>
  void validate_element_type();

  template <typename DIM_T>
  void validate_dimension_type();

  void constructArray(const void *init,
      OSPDataType type,
      uint64_t numItems1,
      int64_t byteStride1 = 0,
      uint64_t numItems2 = 1,
      int64_t byteStride2 = 0,
      uint64_t numItems3 = 1,
      int64_t byteStride3 = 0);
};

using SharedData = Data<true>;
using CopiedData = Data<false>;

static_assert(
    sizeof(Data<>) == sizeof(OSPData), "cpp::Data can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

template <bool SHARED>
template <typename DIM_T>
inline Data<SHARED>::Data(
    const void *init, OSPDataType type, const DIM_T &numItems)
    : Data(init, type, numItems, DIM_T(0))

{}

template <bool SHARED>
template <typename DIM_T>
inline Data<SHARED>::Data(const void *init,
    OSPDataType format,
    const DIM_T &numItems,
    const DIM_T &byteStride)
{
  validate_dimension_type<DIM_T>();

  constexpr auto dim_t = OSPTypeFor<DIM_T>::value;
  constexpr auto nDims = OSPDimensionalityOf<dim_t>::value;

  using vec_element_t = typename OSPVecElementOf<dim_t>::type;

  const auto *d = (const vec_element_t *)&numItems;
  const auto *s = (const vec_element_t *)&byteStride;

  switch (nDims) {
  case 1:
    constructArray(init, format, d[0], s[0]);
    break;
  case 2:
    constructArray(init, format, d[0], s[0], d[1], s[1]);
    break;
  case 3:
    constructArray(init, format, d[0], s[0], d[1], s[1], d[2], s[2]);
    break;
  default:
    throw std::runtime_error("invalid dimension type constructing cpp::Data");
  }
}

template <bool SHARED>
template <typename T, typename DIM_T>
inline Data<SHARED>::Data(const T *init, const DIM_T &numItems)
    : Data(init, numItems, DIM_T(0))
{}

template <bool SHARED>
template <typename T, typename DIM_T>
inline Data<SHARED>::Data(
    const T *init, const DIM_T &numItems, const DIM_T &byteStride)
{
  validate_element_type<T>();
  validate_dimension_type<DIM_T>();

  constexpr auto format = OSPTypeFor<T>::value;
  constexpr auto dim_t = OSPTypeFor<DIM_T>::value;

  constexpr auto nDims = OSPDimensionalityOf<dim_t>::value;

  using vec_element_t = typename OSPVecElementOf<dim_t>::type;

  const auto *d = (const vec_element_t *)&numItems;
  const auto *s = (const vec_element_t *)&byteStride;

  switch (nDims) {
  case 1:
    constructArray(init, format, d[0], s[0]);
    break;
  case 2:
    constructArray(init, format, d[0], s[0], d[1], s[1]);
    break;
  case 3:
    constructArray(init, format, d[0], s[0], d[1], s[1], d[2], s[2]);
    break;
  default:
    throw std::runtime_error("invalid dimension type constructing cpp::Data");
  }
}

template <bool SHARED>
template <typename T, std::size_t N>
inline Data<SHARED>::Data(const std::array<T, N> &arr) : Data(arr.data(), N)
{}

template <bool SHARED>
template <typename T, typename ALLOC_T>
inline Data<SHARED>::Data(const std::vector<T, ALLOC_T> &arr)
    : Data(arr.data(), arr.size())
{}

template <bool SHARED>
template <typename T>
inline Data<SHARED>::Data(const T &obj) : Data(&obj, size_t(1))
{}

template <bool SHARED>
inline Data<SHARED>::Data(OSPData existing)
    : ManagedObject<OSPData, OSP_DATA>(existing)
{}

template <bool SHARED>
template <typename T>
inline void Data<SHARED>::validate_element_type()
{
  static_assert(OSPTypeFor<T>::value != OSP_UNKNOWN,
      "Only types corresponding to OSPDataType values can be set "
      "as elements in OSPRay Data arrays. NOTE: Math types (vec, "
      "box, linear, affine) must be inferrable as OSP_* enums, which are "
      "defined by the OSPTYPEFOR_SPECIALIZATION() macro.");
}

template <bool SHARED>
template <typename DIM_T>
inline void Data<SHARED>::validate_dimension_type()
{
  static_assert(OSPTypeFor<DIM_T>::value == OSP_INT
          || OSPTypeFor<DIM_T>::value == OSP_VEC2I
          || OSPTypeFor<DIM_T>::value == OSP_VEC3I
          || OSPTypeFor<DIM_T>::value == OSP_ULONG
          || OSPTypeFor<DIM_T>::value == OSP_VEC2UL
          || OSPTypeFor<DIM_T>::value == OSP_VEC3UL,
      "The type used to describe an OSPData array dimensions must infer to "
      "OSP_INT, OSP_ULONG, OSP_VEC2UL, or OSP_VEC3UL. These type inferences "
      "are defined by using the OSPTYPEFOR_SPECIALIZATION() macro.");
}

template <bool SHARED>
inline void Data<SHARED>::constructArray(const void *data,
    OSPDataType format,
    uint64_t numItems1,
    int64_t byteStride1,
    uint64_t numItems2,
    int64_t byteStride2,
    uint64_t numItems3,
    int64_t byteStride3)
{
  OSPData tmp = ospNewSharedData(data,
      format,
      numItems1,
      byteStride1,
      numItems2,
      byteStride2,
      numItems3,
      byteStride3);

  if (SHARED) {
    ospObject = tmp;
  } else {
    ospObject = ospNewData(format, numItems1, numItems2, numItems3);
    ospCopyData(tmp, ospObject);
    ospRelease(tmp);
  }
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::SharedData, OSP_DATA);
OSPTYPEFOR_SPECIALIZATION(cpp::CopiedData, OSP_DATA);

} // namespace ospray
