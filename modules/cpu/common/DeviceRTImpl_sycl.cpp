// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DeviceRTImpl_sycl.h"

namespace ospray {
namespace devicert {

/////////////////////////////////////////////////////////////////////
// AsyncEvent implementation

void AsyncEventImpl::wait() const
{
  event.wait();
}

bool AsyncEventImpl::finished() const
{
  return (event.get_info<sycl::info::event::command_execution_status>()
      == sycl::info::event_command_status::complete);
}

float AsyncEventImpl::getDuration() const
{
  try {
    const auto t0 =
        event.get_profiling_info<sycl::info::event_profiling::command_start>();
    const auto t1 =
        event.get_profiling_info<sycl::info::event_profiling::command_end>();
    return (t1 - t0) * 1.0e-9f;
  } catch (...) {
    // In case no profiling data is available
    return 0.f;
  }
}

void *AsyncEventImpl::getSyclEventPtr()
{
  return &event;
}

/////////////////////////////////////////////////////////////////////
// Device implementation

DeviceImpl::DeviceImpl(bool debug)
    : Device(debug),
      device(sycl::gpu_selector_v),
      context(device),
      queue(context,
          device,
          {sycl::property::queue::enable_profiling(),
              sycl::property::queue::in_order()})
{
  postStatusMsg(OSP_LOG_INFO)
      << "Using SYCL GPU device on "
      << device.get_info<sycl::info::device::name>() << " device (default)";
}

namespace {
sycl::device getDeviceByIndex(uint32_t id)
{
  auto gpuDevices = sycl::device::get_devices(sycl::info::device_type::gpu);
  return gpuDevices[id % gpuDevices.size()];
}
} // namespace

DeviceImpl::DeviceImpl(uint32_t deviceId, bool debug)
    : Device(debug),
      device(getDeviceByIndex(deviceId)),
      context(device),
      queue(context,
          device,
          {sycl::property::queue::enable_profiling(),
              sycl::property::queue::in_order()})
{
  postStatusMsg(OSP_LOG_INFO) << "Using SYCL GPU device on "
                              << device.get_info<sycl::info::device::name>()
                              << " device (number: " << deviceId << ")";
}

DeviceImpl::DeviceImpl(void *devicePtr, void *contextPtr, bool debug)
    : Device(debug),
      device(*static_cast<sycl::device *>(devicePtr)),
      context(*static_cast<sycl::context *>(contextPtr)),
      queue(context,
          device,
          {sycl::property::queue::enable_profiling(),
              sycl::property::queue::in_order()})
{
  postStatusMsg(OSP_LOG_INFO) << "Using SYCL GPU device on "
                              << device.get_info<sycl::info::device::name>()
                              << " device (provided externally)";
}

void *DeviceImpl::deviceMalloc(std::size_t size)
{
  return sycl::malloc_device(size, queue);
}

void *DeviceImpl::sharedMalloc(std::size_t size)
{
  return sycl::malloc_shared(
      size, queue, sycl::ext::oneapi::property::usm::device_read_only());
}

void *DeviceImpl::hostMalloc(std::size_t size)
{
  return sycl::malloc_host(size, queue);
}

void DeviceImpl::free(void *ptr)
{
  sycl::free(ptr, queue);
}

Alloc DeviceImpl::getPointerType(void *ptr) const
{
  sycl::usm::alloc type = sycl::get_pointer_type(ptr, queue.get_context());
  switch (type) {
  case sycl::usm::alloc::host:
    return Alloc::Host;
  case sycl::usm::alloc::device:
    return Alloc::Device;
  case sycl::usm::alloc::shared:
    return Alloc::Shared;
  case sycl::usm::alloc::unknown:
    return Alloc::Unknown;
  }
  return Alloc::Unknown;
}

void DeviceImpl::wait()
{
  queue.wait();
}

AsyncEvent DeviceImpl::createAsyncEvent()
{
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::memcpy(void *dest, const void *src, std::size_t size)
{
  // Create async event and do memory copying
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  eventImpl->event = queue.memcpy(dest, src, size);
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchRendererKernel(const vec3ui &itemDims,
    RendererKernel kernel,
    ispc::Renderer *renderer,
    ispc::FrameBuffer *fb,
    ispc::Camera *camera,
    ispc::World *world,
    const uint32_t *taskIDs,
    const FeatureFlags &ff)
{
  // Create async event and launch a kernel
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  kernel(&queue,
      &eventImpl->event,
      itemDims,
      renderer,
      fb,
      camera,
      world,
      taskIDs,
      ff);
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchFrameOpKernel(const vec2ui &itemDims,
    FrameOpKernel kernel,
    const ispc::FrameBufferView *fbv)
{
  // Create async event and launch a kernel
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  kernel(&queue, &eventImpl->event, itemDims, fbv);
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchHostTask(const std::function<void()> &task)
{
  // Create async event and launch a task
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  eventImpl->event = queue.submit(
      [&](sycl::handler &cgh) { cgh.host_task([=]() { task(); }); });
  return AsyncEvent(eventImpl);
}

void *DeviceImpl::getSyclDevicePtr()
{
  // Return SYCL device pointer
  return &device;
}

void *DeviceImpl::getSyclContextPtr()
{
  // Return SYCL context pointer
  return &context;
}

void *DeviceImpl::getSyclQueuePtr()
{
  // Return SYCL command queue pointer
  return &queue;
}

} // namespace devicert
} // namespace ospray
