// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdlib>
#include <new>
#include <type_traits>

#include "rkcommon/common.h"
#include "rkcommon/math/rkmath.h"
#include "rkcommon/math/vec.h"

#include "ISPCRTBuffers.h"

namespace ispc {

// Shared structure members may use types from rkcommon
using namespace rkcommon;
using namespace rkcommon::math;

// Shared structure members may use ispc specific types
using uint8 = uint8_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
} // namespace ispc

namespace ospray {

/*
   Usage:
    derive from AddStructShared<cpp::Base, ispc::StructShared>

   We use multiple inheritance with a virtual base class, thus only a single
   instance of structSharedView is present, which will be initialized first.
   StructSharedGet adds getSh returning the correctly typed pointer. It is
   derived from first to handle the memory allocation with the maximum size of
   the final StructShared. StructSharedGet does not have any data to ensure
   final classes can be C-style casted to ManagedObject* (this pointer stays
   the same).
*/

template <typename T>
inline ISPCRTMemoryView StructSharedCreate(ISPCRTContext context)
{
  ISPCRTMemoryView view =
      BufferSharedCreate(context, sizeof(T), ISPCRT_SM_HOST_WRITE_DEVICE_READ);
  new (ispcrtSharedPtr(view)) T;
  return view;
}

struct StructSharedView
{
  ~StructSharedView();

  template <typename, typename>
  friend struct StructSharedGet;

  template <typename, typename>
  friend struct AddStructShared;

 private:
  ISPCRTMemoryView _view{nullptr};
};

template <typename T, typename>
struct StructSharedGet
{
  StructSharedGet(ISPCRTContext, ISPCRTMemoryView *);
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
      public virtual StructSharedView
{
  using StructShared_t = Struct;
  using StructSharedGet<Struct, AddStructShared<Base, Struct>>::getSh;
  static_assert(
      std::is_same<typename get_base_structshared_or<Base, Struct>::type,
          typename get_super_or<Struct>::type>::value,
      "StructShared_t needs to have 'super' member of type Base::StructShared_t");

  template <typename... Args>
  AddStructShared(ispcrt::Context &context, Args &&...args)
      : StructSharedGet<Struct, AddStructShared<Base, Struct>>(
          context.handle(), &_view),
        Base(std::forward<Args>(args)...)
  {}
};

// Inlined definitions ////////////////////////////////////////

inline StructSharedView::~StructSharedView()
{
  BufferSharedDelete(_view);
}

template <typename T, typename B>
StructSharedGet<T, B>::StructSharedGet(
    ISPCRTContext device, ISPCRTMemoryView *view)
{
  if (!*view)
    *view = StructSharedCreate<T>(device);
}

template <typename T, typename B>
T *StructSharedGet<T, B>::getSh() const
{
  return static_cast<T *>(ispcrtHostPtr(static_cast<const B *>(this)->_view));
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
