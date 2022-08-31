// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstring>
#include <memory>
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

  // TODO: We should move these up into the ISPCRT wrapper
  T *data();

  T *begin();
  T *end();

  const T *cbegin() const;
  const T *cend() const;

  T &operator[](const size_t i);
  const T &operator[](const size_t i) const;
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

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace ospray
