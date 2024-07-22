// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef OSPRAY_TARGET_SYCL
#include "DeviceRTImpl_sycl.h"
#else
#include "DeviceRTImpl_ispc.h"
#endif

namespace ospray {
namespace devicert {

template <typename... Args>
inline std::unique_ptr<Device> make_device_unique(Args &&...args)
{
  return std::unique_ptr<Device>(new DeviceImpl(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferDeviceShadowed<T>>
make_buffer_device_shadowed_unique(Args &&...args)
{
  return std::unique_ptr<BufferDeviceShadowed<T>>(
      new BufferDeviceShadowedImpl<T>(std::forward<Args>(args)...));
}

} // namespace devicert
} // namespace ospray
