// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstring>
#include <memory>
#include <vector>
#include "ispcrt.hpp"

namespace ospray {

/////////////////////////////////////////////////////////////////////
// BufferDevice

template <typename T>
struct BufferDevice : public ispcrt::Array<T, ispcrt::AllocType::Device>
{
  using ispcrt::Array<T, ispcrt::AllocType::Device>::devicePtr;
  BufferDevice(ispcrt::Device &device, T *hostPtr = nullptr);
  BufferDevice(ispcrt::Device &device, size_t size);
};

template <typename T>
BufferDevice<T>::BufferDevice(ispcrt::Device &device, T *hostPtr)
    : ispcrt::Array<T, ispcrt::AllocType::Device>(device, hostPtr)
{}

template <typename T>
BufferDevice<T>::BufferDevice(ispcrt::Device &device, size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Device>(device, nullptr, size)
{}

template <typename T, typename... Args>
inline std::unique_ptr<BufferDevice<T>> make_buffer_device_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferDevice<T>>(
      new BufferDevice<T>(std::forward<Args>(args)...));
}

/////////////////////////////////////////////////////////////////////
// BufferDeviceShadowed

template <typename T>
struct BufferDeviceShadowed : public std::vector<T>,
                              public ispcrt::Array<T, ispcrt::AllocType::Device>
{
  using ispcrt::Array<T, ispcrt::AllocType::Device>::devicePtr;
  using std::vector<T>::size;
  BufferDeviceShadowed(ispcrt::Device &device, size_t size);
  BufferDeviceShadowed(ispcrt::Device &device, std::vector<T> &v);
  BufferDeviceShadowed(ispcrt::Device &device, T *data, size_t size);
};

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    ispcrt::Device &device, size_t size)
    : std::vector<T>(size),
      ispcrt::Array<T, ispcrt::AllocType::Device>(
          device, std::vector<T>::data(), size)
{}

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    ispcrt::Device &device, std::vector<T> &v)
    : std::vector<T>(v),
      ispcrt::Array<T, ispcrt::AllocType::Device>(
          device, std::vector<T>::data(), v.size())
{}

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    ispcrt::Device &device, T *data, size_t size)
    : std::vector<T>(data, data + size),
      ispcrt::Array<T, ispcrt::AllocType::Device>(
          device, std::vector<T>::data(), size)
{}

template <typename T, typename... Args>
inline std::unique_ptr<BufferDeviceShadowed<T>>
make_buffer_device_shadowed_unique(Args &&...args)
{
  return std::unique_ptr<BufferDeviceShadowed<T>>(
      new BufferDeviceShadowed<T>(std::forward<Args>(args)...));
}

/////////////////////////////////////////////////////////////////////
// BufferShared C version

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

/////////////////////////////////////////////////////////////////////
// BufferShared C++ version

template <typename T>
struct BufferShared : public ispcrt::Array<T, ispcrt::AllocType::Shared>
{
  using ispcrt::Array<T, ispcrt::AllocType::Shared>::size;
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
{}

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
  return sharedPtr();
}

template <typename T>
T *BufferShared<T>::begin()
{
  return data();
}

template <typename T>
T *BufferShared<T>::end()
{
  return begin() + size();
}

template <typename T>
const T *BufferShared<T>::cbegin() const
{
  return data();
}

template <typename T>
const T *BufferShared<T>::cend() const
{
  return cbegin() + size();
}

template <typename T>
T &BufferShared<T>::operator[](const size_t i)
{
  return *(data() + i);
}

template <typename T>
const T &BufferShared<T>::operator[](const size_t i) const
{
  return *(data() + i);
}

// The below method is WA for Level Zero bug, when running on GPU sharedPtr()
// crashes on 0-sized ispcrt::Array
template <typename T>
T *BufferShared<T>::sharedPtr() const
{
  return size() ? ispcrt::Array<T, ispcrt::AllocType::Shared>::sharedPtr()
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
