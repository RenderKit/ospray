// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// oidn
#include "OpenImageDenoise/oidn.hpp"
// ospray
#include "fb/ImageOp.h"
#include "ospray_module_denoiser_export.h"

namespace ospray {

struct OSPRAY_MODULE_DENOISER_EXPORT DenoiseFrameOp : public FrameOpInterface
{
  DenoiseFrameOp(api::Device &device);

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;

 private:
  oidn::DeviceRef oidnDevice;
  bool sharedMem{false};
};

} // namespace ospray
