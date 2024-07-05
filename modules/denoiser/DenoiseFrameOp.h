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
  DenoiseFrameOp(devicert::Device &device);

  void commit() override;

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;

  devicert::Device &drtDevice;
  oidn::DeviceRef oidnDevice;
  int quality{OSP_DENOISER_QUALITY_MEDIUM};
  bool denoiseAlpha{false};
};

} // namespace ospray
