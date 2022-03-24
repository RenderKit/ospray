// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "ispcrt.hpp"

namespace ospray {

// C version ////////////////////////////////////////////

inline ISPCRTMemoryView BufferSharedCreate(size_t size)
{
  api::ISPCDevice *device = (api::ISPCDevice *)api::Device::current.ptr;
  ispcrt::Device &ispcrtDevice = device->getIspcrtDevice();

  ISPCRTNewMemoryViewFlags flags;
  flags.allocType = ISPCRT_ALLOC_TYPE_SHARED;
  return ispcrtNewMemoryView(ispcrtDevice.handle(), nullptr, size, &flags);
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
  BufferShared();
  BufferShared(size_t size);
  BufferShared(const std::vector<T> &v);
  BufferShared(const T *data, size_t size);
};

template <typename T>
BufferShared<T>::BufferShared()
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        ((api::ISPCDevice *)api::Device::current.ptr)->getIspcrtDevice())
{}

template <typename T>
BufferShared<T>::BufferShared(size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        ((api::ISPCDevice *)api::Device::current.ptr)->getIspcrtDevice(), size)
{}

template <typename T>
BufferShared<T>::BufferShared(const std::vector<T> &v)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        ((api::ISPCDevice *)api::Device::current.ptr)->getIspcrtDevice(),
        v.size())
{
  memcpy(sharedPtr(), v.data(), sizeof(T) * v.size());
}

template <typename T>
BufferShared<T>::BufferShared(const T *data, size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        ((api::ISPCDevice *)api::Device::current.ptr)->getIspcrtDevice(), size)
{
  memcpy(sharedPtr(), data, sizeof(T) * size);
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace ospray
