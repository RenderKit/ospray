// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
        throw std::runtime_error("Error running OIDN: " +
                                 std::string(errorMessage));
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

  OSP_REGISTER_IMAGE_OP(DenoiseFrameOp, frame_denoise);

}  // namespace ospray

extern "C" OSPRAY_MODULE_DENOISER_EXPORT void ospray_init_module_denoiser() {}
