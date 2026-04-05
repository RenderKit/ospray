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
DeviceImpl::~DeviceImpl()
{

    for (auto &entry : imageMemCache)
      syclexp::free_image_mem(entry.second.memHandle, syclexp::image_type::standard, queue);
    imageMemCache.clear();
    for (auto &h : sampledHandleCache)
      syclexp::destroy_image_handle(h, queue);
    sampledHandleCache.clear();

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

void *DeviceImpl::createImageMemHandle(void **hostData,
    const size_t width,
    const size_t height,
    const unsigned int numLevels,
    const OSPTextureFormat format)
{
  std::cout << "DeviceRTImpl_sycl::createImageMemHandle("
            << "width=" << width << ", height=" << height
            << ", numLevels=" << numLevels << ", format=" << format
            << ")" << std::endl;
  if (!hostData) {
    std::cerr << "ERROR: createImageMemHandle hostData is null" << std::endl;
    return nullptr;
  }
  for (unsigned int i = 0; i < numLevels; ++i) {
    if (!hostData[i]) {
      std::cerr << "ERROR: hostData[" << i << "] is null" << std::endl;
      return nullptr;
    }
  }

  // Determine number of channels and channel data type based on the texture format
  size_t numChannels = 0;
  sycl::image_channel_type channelType;

  switch (format) {
    case OSP_TEXTURE_RGBA8:
    case OSP_TEXTURE_SRGBA:
      numChannels = 4;
      channelType = sycl::image_channel_type::unorm_int8;
    break;
    case OSP_TEXTURE_RGBA32F:
      numChannels = 4;
      channelType = sycl::image_channel_type::fp32;
      break;
    case OSP_TEXTURE_RGBA16:
      numChannels = 4;
      channelType = sycl::image_channel_type::unorm_int16;
      break;
    case OSP_TEXTURE_RGBA16F:
      numChannels = 4;
      channelType = sycl::image_channel_type::fp16;
      break;
    case OSP_TEXTURE_RGB8:
    case OSP_TEXTURE_SRGB:
      numChannels = 3;
      channelType = sycl::image_channel_type::unorm_int8;
      break;
    case OSP_TEXTURE_RGB32F:
      numChannels = 3;
      channelType = sycl::image_channel_type::fp32;
      break;
    case OSP_TEXTURE_RGB16:
      numChannels = 3;
      channelType = sycl::image_channel_type::unorm_int16;
      break;
    case OSP_TEXTURE_RGB16F:
      numChannels = 3;
      channelType = sycl::image_channel_type::fp16;
      break;
    case OSP_TEXTURE_RA8:
    case OSP_TEXTURE_LA8:
      numChannels = 2;
      channelType = sycl::image_channel_type::unorm_int8;
      break;
    case OSP_TEXTURE_RA32F:
      numChannels = 2;
      channelType = sycl::image_channel_type::fp32;
      break;
    case OSP_TEXTURE_RA16:
      numChannels = 2;
      channelType = sycl::image_channel_type::unorm_int16;
      break;
    case OSP_TEXTURE_RA16F:
      numChannels = 2;
      channelType = sycl::image_channel_type::fp16;
      break;
    case OSP_TEXTURE_R8:
    case OSP_TEXTURE_L8:
      numChannels = 1;
      channelType = sycl::image_channel_type::unorm_int8;
      break;
    case OSP_TEXTURE_R32F:
      numChannels = 1;
      channelType = sycl::image_channel_type::fp32;
      break;
    case OSP_TEXTURE_R16:
      numChannels = 1;
      channelType = sycl::image_channel_type::unorm_int16;
      break;
    case OSP_TEXTURE_R16F:
      numChannels = 1;
      channelType = sycl::image_channel_type::fp16;
      break;
    default:
      throw std::runtime_error("Unsupported texture format for bindless images");
  }
  // Construct the image descriptor.
  syclexp::image_descriptor imgDesc(
    {width, height},
    numChannels,
    channelType,
    syclexp::image_type::standard);

  syclexp::image_mem_handle memHandle = syclexp::alloc_image_mem(imgDesc, queue);
  queue.ext_oneapi_copy(hostData[0], memHandle, imgDesc);

  ImageMemEntry imgMemEntry;
  imgMemEntry.desc = imgDesc;
  imgMemEntry.memHandle = memHandle;
  imageMemCache[(void*)memHandle.raw_handle] = imgMemEntry;
  queue.wait_and_throw();
  return (void*)memHandle.raw_handle;
}

void DeviceImpl::freeImageMemHandle(void *handle)
{
  syclexp::image_mem_handle memHandle;
  memHandle.raw_handle = (syclexp::sampled_image_handle::raw_image_handle_type)handle;
  syclexp::free_image_mem(memHandle, syclexp::image_type::standard, queue);
  imageMemCache.erase(handle);
}

void *DeviceImpl::createSampledImageHandle(
    void *imgMemHandlePtr, const OSPTextureFilter filter, const vec2ui wrapMode)
{
    std::cout<<"createSampledImageHandle "<<std::endl;
    sycl::addressing_mode addressingMode;
    switch (wrapMode.x) {
    case OSP_TEXTURE_WRAP_REPEAT:
        addressingMode = sycl::addressing_mode::repeat;
        break;
    case OSP_TEXTURE_WRAP_MIRRORED_REPEAT:
        addressingMode = sycl::addressing_mode::mirrored_repeat;
        break;
    case OSP_TEXTURE_WRAP_CLAMP_TO_EDGE:
        addressingMode = sycl::addressing_mode::clamp_to_edge;
        break;
    default:
        addressingMode = sycl::addressing_mode::repeat;
    }

    sycl::filtering_mode filteringMode = (filter == OSP_TEXTURE_FILTER_NEAREST)
        ? sycl::filtering_mode::nearest
        : sycl::filtering_mode::linear;

    syclexp::bindless_image_sampler sampler(
        addressingMode,
        sycl::coordinate_normalization_mode::normalized,
        filteringMode,
        filteringMode,
        0.f,
        static_cast<float>(32),
        0.f);
    //Get the image descriptor for this image handle
    syclexp::image_descriptor imgDesc = imageMemCache[imgMemHandlePtr].desc;
    //Rebuild the image handle from the pointer
    syclexp::image_mem_handle memHandle;
    memHandle.raw_handle = (syclexp::sampled_image_handle::raw_image_handle_type)imgMemHandlePtr;

    syclexp::sampled_image_handle sampledHandle =
        syclexp::create_image(memHandle, sampler, imgDesc, queue);
    sampledHandleCache.push_back(sampledHandle);
    return reinterpret_cast<void *>(sampledHandle.raw_handle);
}

void DeviceImpl::freeSampledImageHandle(void *handle) {
    syclexp::sampled_image_handle sampledHandle;
    sampledHandle.raw_handle = reinterpret_cast<syclexp::sampled_image_handle::raw_image_handle_type>(handle);
    syclexp::destroy_image_handle(sampledHandle, queue);
}

} // namespace devicert
} // namespace ospray
