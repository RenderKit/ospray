// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/DeviceRT.h"

#include <sycl/sycl.hpp>

namespace ospray {
namespace devicert {

/////////////////////////////////////////////////////////////////////
// AsyncEvent interface

struct OSPRAY_SDK_INTERFACE AsyncEventImpl : public AsyncEventIfc
{
  // Wait for the event
  void wait() const override;

  // Return true if finished
  bool finished() const override;

  // Get event duration
  float getDuration() const override;

  // Allow DeviceImpl to access event
  friend struct DeviceImpl;

  // Get SYCL event pointer
  void *getSyclEventPtr() override;

 private:
  mutable sycl::event event;
};

/////////////////////////////////////////////////////////////////////
// Device interface

struct OSPRAY_SDK_INTERFACE DeviceImpl : public Device
{
  // Construction & destruction
  DeviceImpl(bool debug);
  DeviceImpl(uint32_t deviceId, bool debug);
  DeviceImpl(void *devicePtr, void *contextPtr, bool debug);

  // Allocate device memory
  void *deviceMalloc(std::size_t size) override;

  // Allocate shared memory
  void *sharedMalloc(std::size_t size) override;

  // Allocate host memory
  void *hostMalloc(std::size_t size) override;

  // Release shared or device memory
  void free(void *ptr) override;

  // Get pointer allocation type
  Alloc getPointerType(void *ptr) const override;

  // Wait for device to finish its pending work
  void wait() override;

  // Create asynchronous event object
  AsyncEvent createAsyncEvent() override;

  // Copy memory from/to device
  AsyncEvent memcpy(void *dest, const void *src, std::size_t size) override;

  // Launch a kernel on a device
  AsyncEvent launchRendererKernel(const vec3ui &itemDims,
      RendererKernel kernel,
      ispc::Renderer *renderer,
      ispc::FrameBuffer *fb,
      ispc::Camera *camera,
      ispc::World *world,
      const uint32_t *taskIDs,
      const FeatureFlags &ff) override;
  AsyncEvent launchFrameOpKernel(const vec2ui &itemDims,
      FrameOpKernel kernel,
      const ispc::FrameBufferView *fbv) override;

  // Launch a given task on a host using device queue ordering
  AsyncEvent launchHostTask(const std::function<void()> &task) override;

  // For code (e.g. OIDN API) that is able to use SYCL without including it
  void *getSyclDevicePtr() override;
  void *getSyclContextPtr() override;
  void *getSyclQueuePtr() override;

 private:
  sycl::device device;
  sycl::context context;
  sycl::queue queue;
};

/////////////////////////////////////////////////////////////////////
// Buffer in device memory with system memory shadow

template <typename T>
struct BufferDeviceShadowedImpl : public BufferDeviceShadowed<T>
{
  // Bring these members into scope of derived class
  using BufferDeviceShadowed<T>::device;
  using BufferDeviceShadowed<T>::devPtr;

  // Inherit constructors
  BufferDeviceShadowedImpl(Device &device, std::size_t count);
  BufferDeviceShadowedImpl(Device &device, const std::vector<T> &v);
  BufferDeviceShadowedImpl(Device &device, T *data, std::size_t count);
  ~BufferDeviceShadowedImpl() override;

 protected:
  // Copy between device and shadow buffers
  AsyncEvent memcpy(T *dest, const T *src, std::size_t count) override;
};

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, std::size_t count)
    : BufferDeviceShadowed<T>(
        device, static_cast<T *>(device.deviceMalloc(count * sizeof(T))), count)
{}

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, const std::vector<T> &v)
    : BufferDeviceShadowed<T>(
        device, static_cast<T *>(device.deviceMalloc(v.size() * sizeof(T))), v)
{}

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, T *data, std::size_t count)
    : BufferDeviceShadowed<T>(device,
        static_cast<T *>(device.deviceMalloc(count * sizeof(T))),
        data,
        count)
{}

template <typename T>
BufferDeviceShadowedImpl<T>::~BufferDeviceShadowedImpl()
{
  device.free(devPtr);
}

template <typename T>
AsyncEvent BufferDeviceShadowedImpl<T>::memcpy(
    T *dest, const T *src, std::size_t count)
{
  return device.memcpy(dest, src, count * sizeof(T));
}

} // namespace devicert
} // namespace ospray
