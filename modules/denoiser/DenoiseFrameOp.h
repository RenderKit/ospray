// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// oidn
#include "OpenImageDenoise/oidn.h"
// ospray
#include "fb/ImageOp.h"
#include "ospray_module_denoiser_export.h"

namespace ospray {

struct OSPRAY_MODULE_DENOISER_EXPORT DenoiseFrameOp : public FrameOp
{
  DenoiseFrameOp();

  ~DenoiseFrameOp() override;

  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;

 private:
  OIDNDevice device;
};

} // namespace ospray
