// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "api/Device.h"
#include "fb/FrameBufferView.h"

namespace ospray {

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOp
    : public LiveFrameOpInterface
{
  LiveDenoiseFrameOp(FrameBufferView &fbView, OIDNDevice oidnDevice)
      : oidnDevice(oidnDevice),
        filter(oidnNewFilter(oidnDevice, "RT")),
        fbView(fbView)
  {
    oidnRetainDevice(oidnDevice);

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

    oidnSetSharedFilterImage(filter,
        "output",
        fbColor,
        OIDN_FORMAT_FLOAT3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        0,
        sizeof(float) * 4,
        0);

    oidnSetFilterBool(filter, "hdr", false);

    oidnCommitFilter(filter);
  }

  ~LiveDenoiseFrameOp() override
  {
    oidnReleaseFilter(filter);
    oidnReleaseDevice(oidnDevice);
  }

  void process(void *waitEvent, const Camera *) override
  {
    if (waitEvent)
      oidnExecuteSYCLFilterAsync(filter, nullptr, 0, (sycl::event *)waitEvent);
    else
      oidnExecuteFilter(filter);

    const char *errorMessage = nullptr;
    auto error = oidnGetDeviceError(oidnDevice, &errorMessage);

    if (error != OIDN_ERROR_NONE && error != OIDN_ERROR_CANCELLED) {
      throw std::runtime_error(
          "Error running OIDN: " + std::string(errorMessage));
    }
  }

  OIDNDevice oidnDevice;
  OIDNFilter filter;

  FrameBufferView fbView;
};

DenoiseFrameOp::DenoiseFrameOp(api::Device &device)
{
  // Get appropriate SYCL command queue for post-processing from device
  sycl::queue *syclQueuePtr =
      (sycl::queue *)device.getPostProcessingCommandQueuePtr();
  if (syclQueuePtr)
    oidnDevice = oidnNewSYCLDevice(syclQueuePtr, 1);
  else
    oidnDevice = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);

  oidnSetDeviceBool(oidnDevice, "setAffinity", false);
  oidnCommitDevice(oidnDevice);
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

  return rkcommon::make_unique<LiveDenoiseFrameOp>(fbView, oidnDevice);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray