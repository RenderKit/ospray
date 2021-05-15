// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "fb/FrameBuffer.h"

namespace ospray {

static bool osprayDenoiseMonitorCallback(void *userPtr, double)
{
  auto *fb = (FrameBuffer *)userPtr;
  return !fb->frameCancelled();
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOp : public LiveFrameOp
{
  LiveDenoiseFrameOp(FrameBufferView &_fbView, OIDNDevice device)
      : LiveFrameOp(_fbView),
        device(device),
        filter(oidnNewFilter(device, "RT"))
  {
    oidnRetainDevice(device);

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

    oidnSetFilter1b(filter, "hdr", false);

    oidnSetFilterProgressMonitorFunction(filter,
        (OIDNProgressMonitorFunction)osprayDenoiseMonitorCallback,
        _fbView.originalFB);

    oidnCommitFilter(filter);
  }

  ~LiveDenoiseFrameOp() override
  {
    oidnReleaseFilter(filter);
    oidnReleaseDevice(device);
  }

  void process(const Camera *) override
  {
    oidnExecuteFilter(filter);

    const char *errorMessage = nullptr;
    auto error = oidnGetDeviceError(device, &errorMessage);

    if (error != OIDN_ERROR_NONE && error != OIDN_ERROR_CANCELLED) {
      std::cout << "OIDN ERROR " << errorMessage << "\n";
      throw std::runtime_error(
          "Error running OIDN: " + std::string(errorMessage));
    }
  }

  OIDNDevice device;
  OIDNFilter filter;
};

DenoiseFrameOp::DenoiseFrameOp()
    : device(oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT))
{
  oidnSetDevice1b(device, "setAffinity", false);
  oidnCommitDevice(device);
}

DenoiseFrameOp::~DenoiseFrameOp()
{
  oidnReleaseDevice(device);
}

std::unique_ptr<LiveImageOp> DenoiseFrameOp::attach(FrameBufferView &fbView)
{
  if (fbView.colorBufferFormat != OSP_FB_RGBA32F)
    throw std::runtime_error(
        "DenoiseFrameOp must be used with an RGBA32F "
        "color format framebuffer!");

  return rkcommon::make_unique<LiveDenoiseFrameOp>(fbView, device);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_denoiser(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  auto status = ospray::moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR)
    ospray::ImageOp::registerType<ospray::DenoiseFrameOp>("denoiser");

  return status;
}
