// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "common/DeviceRT.h"
#include "fb/FrameBufferView.h"

namespace ospray {

void checkError(oidn::DeviceRef oidnDevice)
{
  const char *errorMessage = nullptr;
  auto error = oidnDevice.getError(errorMessage);
  if (error != oidn::Error::None && error != oidn::Error::Cancelled) {
    throw std::runtime_error(
        "Error running OIDN: " + std::string(errorMessage));
  }
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOp
    : public LiveFrameOpInterface
{
  LiveDenoiseFrameOp(FrameBufferView &fbView,
      devicert::Device &drtDevice,
      oidn::DeviceRef oidnDevice)
      : fbView(fbView),
        drtDevice(drtDevice),
        oidnDevice(oidnDevice),
        filter(oidnDevice.newFilter("RT"))
  {
    filter.set("hdr", true);
  }

 protected:
  FrameBufferView fbView;
  devicert::Device &drtDevice;
  oidn::DeviceRef oidnDevice;
  oidn::FilterRef filter;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpShared
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpShared(FrameBufferView &fbView,
      devicert::Device &drtDevice,
      oidn::DeviceRef oidnDevice)
      : LiveDenoiseFrameOp(fbView, drtDevice, oidnDevice),
        buffer(oidnDevice.newBuffer(fbView.colorBufferOutput,
            fbView.viewDims.long_product() * sizeof(vec4f)))
  {
    // Set up filter
    filter.setImage("color",
        buffer,
        oidn::Format::Float3,
        fbView.viewDims.x,
        fbView.viewDims.y,
        0,
        4 * sizeof(float));
    filter.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.viewDims.x,
        fbView.viewDims.y,
        0,
        4 * sizeof(float));
    if (fbView.normalBuffer)
      filter.setImage("normal",
          const_cast<vec3f *>(fbView.normalBuffer),
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y);
    if (fbView.albedoBuffer)
      filter.setImage("albedo",
          const_cast<vec3f *>(fbView.albedoBuffer),
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y);
    filter.commit();
  }

  devicert::AsyncEvent process() override
  {
    // TODO: Remove oidn::Buffer and copying after switching to OIDN 2.2
    // OIDN cannot denoise alpha in a single pass so we copy input buffer to
    // output and then do in-place denoising to preserve alpha
    devicert::AsyncEvent event;
    if (drtDevice.getSyclQueuePtr()) {
      buffer.writeAsync(0,
          fbView.viewDims.long_product() * sizeof(vec4f),
          fbView.colorBufferInput);

      // Create AsyncEvent underlying implementation object
      event = drtDevice.createAsyncEvent();
      sycl::event *syclEvent = (sycl::event *)event.getSyclEventPtr();

      // Using SYCL call without SYCL, that's supported in C99 API only
      oidnExecuteSYCLFilterAsync(filter.getHandle(), nullptr, 0, syclEvent);
      checkError(oidnDevice);
    } else {
      event = drtDevice.launchHostTask([this]() {
        buffer.write(0,
            fbView.viewDims.long_product() * sizeof(vec4f),
            fbView.colorBufferInput);
        filter.execute();
        checkError(oidnDevice);
      });
    }
    return event;
  }

 private:
  oidn::BufferRef buffer;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpCopy
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpCopy(FrameBufferView &fbView,
      devicert::Device &drtDevice,
      oidn::DeviceRef oidnDevice)
      : LiveDenoiseFrameOp(fbView, drtDevice, oidnDevice)
  {
    byteFloatBufferSize = sizeof(float) * fbView.fbDims.product();
    size_t sz = 4 * byteFloatBufferSize;

    if (fbView.normalBuffer) {
      byteNormalOffset = sz;
      sz += 3 * byteFloatBufferSize;
    }
    if (fbView.albedoBuffer) {
      byteAlbedoOffset = sz;
      sz += 3 * byteFloatBufferSize;
    }
    buffer = oidnDevice.newBuffer(sz, oidn::Storage::Device);

    filter.setImage("color",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        0,
        sizeof(float) * 4);
    filter.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        0,
        sizeof(float) * 4,
        0);
    if (fbView.normalBuffer)
      filter.setImage("normal",
          buffer,
          oidn::Format::Float3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          byteNormalOffset);
    if (fbView.albedoBuffer)
      filter.setImage("albedo",
          buffer,
          oidn::Format::Float3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          byteAlbedoOffset);
    filter.commit();
  }

  devicert::AsyncEvent process() override
  {
    devicert::AsyncEvent event = drtDevice.launchHostTask([this]() {
      // As this path is taken when rendering on CPU and denoising on GPU,
      // the asynchronous OIDN API is used to reduce the number of CPU - GPU
      // synchronization points which would take place on every command in case
      // of using synchronous OIDN API.

      // Copy data to input buffers
      buffer.writeAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferInput);
      if (fbView.normalBuffer)
        buffer.writeAsync(
            byteNormalOffset, 3 * byteFloatBufferSize, fbView.normalBuffer);
      if (fbView.albedoBuffer)
        buffer.writeAsync(
            byteAlbedoOffset, 3 * byteFloatBufferSize, fbView.albedoBuffer);

      // Execute denoising kernel
      filter.executeAsync();

      // Copy denoised data to output buffer
      buffer.readAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferOutput);

      // Wait for the async commands to finish
      oidnDevice.sync();
      checkError(oidnDevice);
    });
    return event;
  }

 private:
  oidn::BufferRef buffer;
  size_t byteFloatBufferSize;
  size_t byteNormalOffset;
  size_t byteAlbedoOffset;
};

DenoiseFrameOp::DenoiseFrameOp(devicert::Device &device) : drtDevice(device)
{
  // Get appropriate SYCL command queue for post-processing from device
  sycl::queue *syclQueuePtr = (sycl::queue *)drtDevice.getSyclQueuePtr();
  if (syclQueuePtr) {
    // Using SYCL call without SYCL, that's supported in C99 API only
    oidnDevice = oidnNewSYCLDevice(syclQueuePtr, 1);
  } else {
    oidnDevice = oidn::newDevice();
  }
  if (device.isDebug())
    oidnDevice.set("verbose", 2);

  oidnDevice.commit();
  checkError(oidnDevice);

  sharedMem = syclQueuePtr || oidnDevice.get<bool>("systemMemorySupported");
}

std::unique_ptr<LiveFrameOpInterface> DenoiseFrameOp::attach(
    FrameBufferView &fbView)
{
  if (sharedMem)
    return rkcommon::make_unique<LiveDenoiseFrameOpShared>(
        fbView, drtDevice, oidnDevice);
  return rkcommon::make_unique<LiveDenoiseFrameOpCopy>(
      fbView, drtDevice, oidnDevice);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray
