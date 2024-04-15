// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/DeviceRT.h"

#include "rkcommon/utility/CodeTimer.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace ospray {
namespace devicert {

/////////////////////////////////////////////////////////////////////
// AsyncEvent interface

struct OSPRAY_SDK_INTERFACE AsyncEventImpl : public AsyncEventIfc
{
  // Signalling job state
  void jobScheduled();
  void jobStarted();
  void jobFinished();

  // Wait for the event
  void wait() const override;

  // Return true if finished
  bool finished() const override;

  // Get event duration
  float getDuration() const override;

  // Get SYCL event pointer
  void *getSyclEventPtr() override;

 private:
  // Event state
  bool ready{true};

  // Measure job time
  rkcommon::utility::CodeTimer timer;

  // Synchronization to wait on event
  mutable std::mutex mtx;
  mutable std::condition_variable cv;
};

/////////////////////////////////////////////////////////////////////
// Device interface

struct OSPRAY_SDK_INTERFACE DeviceImpl : public Device
{
  // Construction & destruction
  DeviceImpl(bool debug);
  DeviceImpl(uint32_t deviceId, bool debug);
  DeviceImpl(void *devicePtr, void *contextPtr, bool debug);
  ~DeviceImpl() override;

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
  // Inner command classes
  class Command;
  class CommandShutdown;
  class CommandMemCpy;
  class CommandLaunchRenderKernel;
  class CommandLaunchFrameOpKernel;
  class CommandRunHostTask;
  using CommandPtr = std::unique_ptr<Command>;

  // Device command queue
  std::queue<CommandPtr> commandQueue;
  std::mutex queueMtx;
  std::condition_variable queueCV;

  // Device worker thread
  std::thread workerThread;

  // Schedule command for execution
  void scheduleCommand(CommandPtr cmd);
};

/////////////////////////////////////////////////////////////////////
// Buffer in device memory with system memory shadow

template <typename T>
struct BufferDeviceShadowedImpl : public BufferDeviceShadowed<T>
{
  // Inherit constructors
  BufferDeviceShadowedImpl(Device &device, std::size_t count);
  BufferDeviceShadowedImpl(Device &device, const std::vector<T> &v);
  BufferDeviceShadowedImpl(Device &device, T *data, std::size_t count);

 protected:
  // Copy between device and shadow buffers
  AsyncEvent memcpy(T *, const T *, std::size_t) override
  {
    return AsyncEvent();
  }
};

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, std::size_t count)
    : BufferDeviceShadowed<T>(device, nullptr, count)
{}

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, const std::vector<T> &v)
    : BufferDeviceShadowed<T>(device, nullptr, v)
{}

template <typename T>
BufferDeviceShadowedImpl<T>::BufferDeviceShadowedImpl(
    Device &device, T *data, std::size_t count)
    : BufferDeviceShadowed<T>(device, nullptr, data, count)
{}

} // namespace devicert
} // namespace ospray
