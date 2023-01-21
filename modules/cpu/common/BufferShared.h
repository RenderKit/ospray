// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstring>
#include <memory>
#include "ispcrt.hpp"

namespace ospray {

// C version ////////////////////////////////////////////

inline ISPCRTMemoryView BufferSharedCreate(ISPCRTContext context,
    size_t size,
    ISPCRTSharedMemoryAllocationHint allocHint =
        ISPCRT_SM_HOST_DEVICE_READ_WRITE)
{
  ISPCRTNewMemoryViewFlags flags;
  flags.allocType = ISPCRT_ALLOC_TYPE_SHARED;
  flags.smHint = allocHint;
  return ispcrtNewMemoryViewForContext(context, nullptr, size, &flags);
}

inline void BufferSharedDelete(ISPCRTMemoryView view)
{
  ispcrtRelease(view);
}

// C++ version ////////////////////////////////////////////

template <typename T>
struct BufferShared : public ispcrt::Array<T, ispcrt::AllocType::Shared>
{
  using ispcrt::Array<T, ispcrt::AllocType::Shared>::sharedPtr;
  BufferShared(ispcrt::Context &context,
      ispcrt::SharedMemoryUsageHint allocHint =
          ispcrt::SharedMemoryUsageHint::HostDeviceReadWrite);
  BufferShared(ispcrt::Context &context,
      size_t size,
      ispcrt::SharedMemoryUsageHint allocHint =
          ispcrt::SharedMemoryUsageHint::HostDeviceReadWrite);
  BufferShared(ispcrt::Context &context,
      const std::vector<T> &v,
      ispcrt::SharedMemoryUsageHint allocHint =
          ispcrt::SharedMemoryUsageHint::HostDeviceReadWrite);
  BufferShared(ispcrt::Context &context,
      const T *data,
      size_t size,
      ispcrt::SharedMemoryUsageHint allocHint =
          ispcrt::SharedMemoryUsageHint::HostDeviceReadWrite);

  // TODO: We should move these up into the ISPCRT wrapper
  T *data();

  T *begin();
  T *end();

  const T *cbegin() const;
  const T *cend() const;

  T &operator[](const size_t i);
  const T &operator[](const size_t i) const;

  T *sharedPtr() const;
};

template <typename T>
BufferShared<T>::BufferShared(
    ispcrt::Context &context, ispcrt::SharedMemoryUsageHint allocHint)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(context, allocHint)
{}

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Context &context,
    size_t size,
    ispcrt::SharedMemoryUsageHint allocHint)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(context, size, allocHint)
{
}

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Context &context,
    const std::vector<T> &v,
    ispcrt::SharedMemoryUsageHint allocHint)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(context, v.size(), allocHint)
{
  std::memcpy(sharedPtr(), v.data(), sizeof(T) * v.size());
}

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Context &context,
    const T *data,
    size_t size,
    ispcrt::SharedMemoryUsageHint allocHint)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(context, size, allocHint)
{
  std::memcpy(sharedPtr(), data, sizeof(T) * size);
}

template <typename T>
T *BufferShared<T>::data()
{
  return begin();
}

template <typename T>
T *BufferShared<T>::begin()
{
  return sharedPtr();
}

template <typename T>
T *BufferShared<T>::end()
{
  return begin() + ispcrt::Array<T, ispcrt::AllocType::Shared>::size();
}

template <typename T>
const T *BufferShared<T>::cbegin() const
{
  return sharedPtr();
}

template <typename T>
const T *BufferShared<T>::cend() const
{
  return cbegin() + ispcrt::Array<T, ispcrt::AllocType::Shared>::size();
}

template <typename T>
T &BufferShared<T>::operator[](const size_t i)
{
  return *(sharedPtr() + i);
}

template <typename T>
const T &BufferShared<T>::operator[](const size_t i) const
{
  return *(sharedPtr() + i);
}

// The below method is WA for ISPCRT bug, when running on GPU sharedPtr()
// crashes on 0-sized ispcrt::Array
// TODO: Fix it in ISPCRT
template <typename T>
T *BufferShared<T>::sharedPtr() const
{
  return ispcrt::Array<T, ispcrt::AllocType::Shared>::size()
      ? ispcrt::Array<T, ispcrt::AllocType::Shared>::sharedPtr()
      : nullptr;
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace ospray
