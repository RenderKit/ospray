// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"

namespace ospray {

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

    oidnCommitFilter(filter);
  }

  ~LiveDenoiseFrameOp()
  {
    oidnReleaseFilter(filter);
    oidnReleaseDevice(device);
  }

  void process(const Camera *)
  {
    oidnExecuteFilter(filter);

    const char *errorMessage = nullptr;
    if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE) {
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

  return ospcommon::make_unique<LiveDenoiseFrameOp>(fbView, device);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

OSP_REGISTER_IMAGE_OP(DenoiseFrameOp, denoiser);

} // namespace ospray

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_denoiser(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  return ospray::moduleVersionCheck(versionMajor, versionMinor);
}
