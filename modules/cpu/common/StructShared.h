// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdlib>
#include <new>
#include <type_traits>

#include "rkcommon/common.h"
#include "rkcommon/math/rkmath.h"
#include "rkcommon/math/vec.h"

#include "DeviceRT.h"

namespace ispc {

// Shared structure members may use types from rkcommon
using namespace rkcommon;
using namespace rkcommon::math;

// Shared structure members may use ispc specific types
using uint8 = uint8_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;

static_assert(sizeof(bool) == 1,
    "Sharing structs with ISPC not possible, because different sizes of 'bool'.");

} // namespace ispc

namespace ospray {

/*
   Usage:
    derive from AddStructShared<cpp::Base, ispc::StructShared>

   We use multiple inheritance with a virtual base class, thus only a single
   instance of StructSharedPtr is present, which will be initialized first.
   StructSharedGet adds getSh returning the correctly typed pointer. It is
   derived from first to handle the memory allocation with the maximum size of
   the final StructShared. StructSharedGet does not have any data to ensure
   final classes can be C-style casted to ManagedObject* (this pointer stays
   the same).
*/

template <typename T>
inline T *StructSharedCreate(devicert::Device &device)
{
  return new (device.sharedMalloc(sizeof(T))) T;
}

struct StructSharedPtr
{
  StructSharedPtr() : _device(nullptr), _ptr(nullptr) {}
  ~StructSharedPtr();

  template <typename, typename>
  friend struct StructSharedGet;

  template <typename, typename>
  friend struct AddStructShared;

 private:
  devicert::Device *_device;
  void *_ptr;
};

template <typename T, typename>
struct StructSharedGet
{
  StructSharedGet(devicert::Device &, devicert::Device **, void **);
  T *getSh() const;
};

// Traits /////////////////////////////////////////////////////

template <class...>
struct make_void
{
  using type = void;
};

template <typename... T>
using void_t = typename make_void<T...>::type;

template <typename, typename S, typename = void>
struct get_base_structshared_or
{
  using type = S;
};

template <typename T, typename S>
struct get_base_structshared_or<T, S, void_t<typename T::StructShared_t>>
{
  using type = typename T::StructShared_t;
};

template <typename S, typename = void>
struct get_super_or
{
  using type = S;
};

template <typename S>
struct get_super_or<S, void_t<decltype(S::super)>>
{
  using type = decltype(S::super);
};

// AddStructShared ////////////////////////////////////////////

template <typename Base, typename Struct>
struct AddStructShared
    : public StructSharedGet<Struct, AddStructShared<Base, Struct>>,
      public Base,
      public virtual StructSharedPtr
{
  using StructShared_t = Struct;
  using StructSharedGet<Struct, AddStructShared<Base, Struct>>::getSh;
  static_assert(
      std::is_same<typename get_base_structshared_or<Base, Struct>::type,
          typename get_super_or<Struct>::type>::value,
      "StructShared_t needs to have 'super' member of type Base::StructShared_t");

  template <typename... Args>
  AddStructShared(devicert::Device &device, Args &&...args)
      : StructSharedGet<Struct, AddStructShared<Base, Struct>>(
          device, &_device, &_ptr),
        Base(std::forward<Args>(args)...)
  {}
};

// Inlined definitions ////////////////////////////////////////

inline StructSharedPtr::~StructSharedPtr()
{
  _device->free(_ptr);
}

template <typename T, typename B>
StructSharedGet<T, B>::StructSharedGet(
    devicert::Device &device, devicert::Device **_device, void **_ptr)
{
  if (!*_ptr) {
    *_ptr = (void *)StructSharedCreate<T>(device);
    *_device = &device;
  }
}

template <typename T, typename B>
T *StructSharedGet<T, B>::getSh() const
{
  return static_cast<T *>(static_cast<const B *>(this)->_ptr);
}

// Testing ////////////////////////////////////////////////////
namespace test {
// clang-format off
struct A {};
struct B { char b; };
struct D1 { B super; };
struct D2 { D1 super; char c; };

struct ShouldPass1 : public AddStructShared<A, B> {};
struct ShouldPass2 : public AddStructShared<ShouldPass1, D1> {};
struct ShouldPass3 : public AddStructShared<ShouldPass2, D2> {};

//struct ShouldFail1 : public AddStructShared<ShouldPass3, D2> {};
//struct ShouldFail2 : public AddStructShared<ShouldPass2, B> {};
//struct ShouldFail3 : public AddStructShared<ShouldPass3, D1> {};
//  clang-format on
} // namespace test

} // namespace ospray
