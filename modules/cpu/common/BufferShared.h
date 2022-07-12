// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispcrt.hpp"

namespace ospray {

// C version ////////////////////////////////////////////

inline ISPCRTMemoryView BufferSharedCreate(ISPCRTDevice device, size_t size)
{
  ISPCRTNewMemoryViewFlags flags;
  flags.allocType = ISPCRT_ALLOC_TYPE_SHARED;
  return ispcrtNewMemoryView(device, nullptr, size, &flags);
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
  BufferShared(ispcrt::Device &device);
  BufferShared(ispcrt::Device &device, size_t size);
  BufferShared(ispcrt::Device &device, const std::vector<T> &v);
  BufferShared(ispcrt::Device &device, const T *data, size_t size);
};

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Device &device)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(device)
{}

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Device &device, size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(device, size)
{}

template <typename T>
BufferShared<T>::BufferShared(ispcrt::Device &device, const std::vector<T> &v)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(device, v.size())
{
  std::memcpy(sharedPtr(), v.data(), sizeof(T) * v.size());
}

template <typename T>
BufferShared<T>::BufferShared(
    ispcrt::Device &device, const T *data, size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(device, size)
{
  std::memcpy(sharedPtr(), data, sizeof(T) * size);
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&... args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace ospray
