// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "api/Device.h"
#include "fb/FrameBufferView.h"

namespace ospray {

void checkError(OIDNDevice oidnDevice)
{
  const char *errorMessage = nullptr;
  auto error = oidnGetDeviceError(oidnDevice, &errorMessage);

  if (error != OIDN_ERROR_NONE && error != OIDN_ERROR_CANCELLED) {
    throw std::runtime_error(
        "Error running OIDN: " + std::string(errorMessage));
  }
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOp
    : public LiveFrameOpInterface
{
  LiveDenoiseFrameOp(FrameBufferView &fbView, OIDNDevice oidnDevice)
      : fbView(fbView),
        oidnDevice(oidnDevice),
        filter(oidnNewFilter(oidnDevice, "RT"))

  {
    oidnRetainDevice(oidnDevice);
    oidnSetFilterBool(filter, "hdr", true);
  }

  ~LiveDenoiseFrameOp() override
  {
    oidnReleaseFilter(filter);
    oidnReleaseDevice(oidnDevice);
  }

  void process(void *waitEvent, const Camera *) override
  {
    if (!filterCommited) {
      float *fbColor = static_cast<float *>(fbView.colorBuffer);
      oidnSetSharedFilterImage(filter,
          "color",
          fbColor,
          OIDN_FORMAT_FLOAT3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          0,
          sizeof(float) * 4,
          0);

      oidnSetSharedFilterImage(filter,
          "output",
          fbColor,
          OIDN_FORMAT_FLOAT3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          0,
          sizeof(float) * 4,
          0);

      if (fbView.normalBuffer)
        oidnSetSharedFilterImage(filter,
            "normal",
            fbView.normalBuffer,
            OIDN_FORMAT_FLOAT3,
            fbView.fbDims.x,
            fbView.fbDims.y,
            0,
            0,
            0);

      if (fbView.albedoBuffer)
        oidnSetSharedFilterImage(filter,
            "albedo",
            fbView.albedoBuffer,
            OIDN_FORMAT_FLOAT3,
            fbView.fbDims.x,
            fbView.fbDims.y,
            0,
            0,
            0);

      oidnCommitFilter(filter);
      filterCommited = true;
    }

    if (waitEvent)
      oidnExecuteSYCLFilterAsync(filter, nullptr, 0, (sycl::event *)waitEvent);
    else
      oidnExecuteFilter(filter);

    checkError(oidnDevice);
  }

  FrameBufferView fbView;
  bool filterCommited{false};
  OIDNDevice oidnDevice;
  OIDNFilter filter;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpCopy
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpCopy(FrameBufferView &fbView, OIDNDevice oidnDevice)
      : LiveDenoiseFrameOp(fbView, oidnDevice)
  {}

  ~LiveDenoiseFrameOpCopy() override
  {
    oidnReleaseBuffer(input);
    oidnReleaseBuffer(output);
  }

  void process(void * /*waitEvent*/, const Camera *) override
  {
    if (!filterCommited) {
      byteFloatBufferSize = sizeof(float) * fbView.fbDims.product();
      size_t sz = 4 * byteFloatBufferSize;
      output = oidnNewBufferWithStorage(oidnDevice, sz, OIDN_STORAGE_DEVICE);

      if (fbView.normalBuffer) {
        byteNormalOffset = sz;
        sz += 3 * byteFloatBufferSize;
      }
      if (fbView.albedoBuffer) {
        byteAlbedoOffset = sz;
        sz += 3 * byteFloatBufferSize;
      }
      input = oidnNewBufferWithStorage(oidnDevice, sz, OIDN_STORAGE_DEVICE);

      oidnSetFilterImage(filter,
          "color",
          input,
          OIDN_FORMAT_FLOAT3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          0,
          sizeof(float) * 4,
          0);

      oidnSetFilterImage(filter,
          "output",
          output,
          OIDN_FORMAT_FLOAT3,
          fbView.fbDims.x,
          fbView.fbDims.y,
          0,
          sizeof(float) * 4,
          0);

      if (fbView.normalBuffer)
        oidnSetFilterImage(filter,
            "normal",
            input,
            OIDN_FORMAT_FLOAT3,
            fbView.fbDims.x,
            fbView.fbDims.y,
            byteNormalOffset,
            0,
            0);

      if (fbView.albedoBuffer)
        oidnSetFilterImage(filter,
            "albedo",
            input,
            OIDN_FORMAT_FLOAT3,
            fbView.fbDims.x,
            fbView.fbDims.y,
            byteAlbedoOffset,
            0,
            0);

      oidnCommitFilter(filter);
      filterCommited = true;
    }
// TODO if (!fbView.originalFB->getSh()->numPixelsRendered)
//        return; // skip denoising when no new pixels XXX only works on CPU

    oidnWriteBufferAsync(input, 0, 4 * byteFloatBufferSize, fbView.colorBuffer);

    if (fbView.normalBuffer)
      oidnWriteBufferAsync(input,
          byteNormalOffset,
          3 * byteFloatBufferSize,
          fbView.normalBuffer);
    if (fbView.albedoBuffer)
      oidnWriteBufferAsync(input,
          byteAlbedoOffset,
          3 * byteFloatBufferSize,
          fbView.albedoBuffer);

    oidnExecuteFilterAsync(filter);

    oidnReadBufferAsync(output, 0, 4 * byteFloatBufferSize, fbView.colorBuffer);

    oidnSyncDevice(oidnDevice);

    checkError(oidnDevice);
  }

  OIDNBuffer input;
  OIDNBuffer output;
  size_t byteFloatBufferSize;
  size_t byteNormalOffset;
  size_t byteAlbedoOffset;
};

DenoiseFrameOp::DenoiseFrameOp(api::Device &device)
{
  // Get appropriate SYCL command queue for post-processing from device
  sycl::queue *syclQueuePtr =
      (sycl::queue *)device.getPostProcessingCommandQueuePtr();
  if (syclQueuePtr)
    oidnDevice = oidnNewSYCLDevice(syclQueuePtr, 1);
  else
    oidnDevice = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);

  checkError(oidnDevice);

  if (device.debugMode)
    oidnSetDeviceInt(oidnDevice, "verbose", 2);

  oidnCommitDevice(oidnDevice);

  sharedMem =
      syclQueuePtr || oidnGetDeviceBool(oidnDevice, "systemMemorySupported");
}

DenoiseFrameOp::~DenoiseFrameOp()
{
  oidnReleaseDevice(oidnDevice);
}

std::unique_ptr<LiveFrameOpInterface> DenoiseFrameOp::attach(
    FrameBufferView &fbView)
{
  if (fbView.colorBufferFormat != OSP_FB_RGBA32F)
    throw std::runtime_error(
        "DenoiseFrameOp must be used with an RGBA32F "
        "color format framebuffer!");

  return sharedMem
      ? rkcommon::make_unique<LiveDenoiseFrameOp>(fbView, oidnDevice)
      : rkcommon::make_unique<LiveDenoiseFrameOpCopy>(fbView, oidnDevice);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray
