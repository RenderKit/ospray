// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "api/Device.h"
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
  LiveDenoiseFrameOp(FrameBufferView &fbView, oidn::DeviceRef oidnDevice)
      : fbView(fbView),
        oidnDevice(oidnDevice),
        filter(oidnDevice.newFilter("RT"))
  {
    filter.set("hdr", true);
  }

 protected:
  FrameBufferView fbView;
  oidn::DeviceRef oidnDevice;
  oidn::FilterRef filter;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpShared
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpShared(FrameBufferView &fbView, oidn::DeviceRef oidnDevice)
      : LiveDenoiseFrameOp(fbView, oidnDevice),
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

  void process(void *waitEvent) override
  {
    // TODO: Remove oidn::Buffer and copying after switching to OIDN 2.2
    // OIDN cannot denoise alpha in a single pass so we copy input buffer to
    // output and then do in-place denoising to preserve alpha
    if (waitEvent) {
      buffer.writeAsync(0,
          fbView.viewDims.long_product() * sizeof(vec4f),
          fbView.colorBufferInput);

      // Using SYCL call without SYCL, that's supported in C99 API only
      oidnExecuteSYCLFilterAsync(
          filter.getHandle(), nullptr, 0, (sycl::event *)waitEvent);
    } else {
      buffer.write(0,
          fbView.viewDims.long_product() * sizeof(vec4f),
          fbView.colorBufferInput);
      filter.execute();
    }

    checkError(oidnDevice);
  }

 private:
  oidn::BufferRef buffer;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpCopy
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpCopy(FrameBufferView &fbView, oidn::DeviceRef oidnDevice)
      : LiveDenoiseFrameOp(fbView, oidnDevice)
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

  void process(void * /*waitEvent*/) override
  {
    // TODO if (!fbView.originalFB->getSh()->numPixelsRendered)
    //        return; // skip denoising when no new pixels XXX only works on CPU

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
    oidnDevice.sync();

    checkError(oidnDevice);
  }

 private:
  oidn::BufferRef buffer;
  size_t byteFloatBufferSize;
  size_t byteNormalOffset;
  size_t byteAlbedoOffset;
};

DenoiseFrameOp::DenoiseFrameOp(api::Device &device)
{
  // Get appropriate SYCL command queue for post-processing from device
  sycl::queue *syclQueuePtr =
      (sycl::queue *)device.getPostProcessingCommandQueuePtr();
  if (syclQueuePtr) {
    // Using SYCL call without SYCL, that's supported in C99 API only
    oidnDevice = oidnNewSYCLDevice(syclQueuePtr, 1);
  } else {
    oidnDevice = oidn::newDevice();
  }
  checkError(oidnDevice);

  if (device.debugMode)
    oidnDevice.set("verbose", 2);

  oidnDevice.commit();

  sharedMem = syclQueuePtr || oidnDevice.get<bool>("systemMemorySupported");
}

std::unique_ptr<LiveFrameOpInterface> DenoiseFrameOp::attach(
    FrameBufferView &fbView)
{
  if (sharedMem)
    return rkcommon::make_unique<LiveDenoiseFrameOpShared>(fbView, oidnDevice);
  return rkcommon::make_unique<LiveDenoiseFrameOpCopy>(fbView, oidnDevice);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray
