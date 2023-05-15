// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ToneMapper.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
#include "fb/pixel_ops/ToneMapper_ispc.h"
#else
namespace ispc {
void *LiveToneMapper_processPixel_addr();
}
#endif

using namespace rkcommon;

namespace ospray {

void ToneMapper::commit()
{
  ImageOp::commit();

  exposure = getParam<float>("exposure", 1.f);
  // Default parameters fitted to the ACES 1.0 grayscale curve
  // (RRT.a1.0.3 + ODT.Academy.Rec709_100nits_dim.a1.0.3)
  // We included exposure adjustment to match 18% middle gray
  // (ODT(RRT(0.18)) = 0.18)
  const float aces_contrast = 1.6773f;
  const float aces_shoulder = 0.9714f;
  const float aces_midIn = 0.18f;
  const float aces_midOut = 0.18f;
  const float aces_hdrMax = 11.0785f;

  a = max(getParam<float>("contrast", aces_contrast), 0.0001f);
  d = clamp(getParam<float>("shoulder", aces_shoulder), 0.0001f, 1.f);

  float m = clamp(getParam<float>("midIn", aces_midIn), 0.0001f, 1.f);
  float n = clamp(getParam<float>("midOut", aces_midOut), 0.0001f, 1.f);

  float w = max(getParam<float>("hdrMax", aces_hdrMax), 1.f);
  acesColor = getParam<bool>("acesColor", true);

  // Solve b and c
  b = -((powf(m, -a * d)
            * (-powf(m, a)
                + (n
                      * (powf(m, a * d) * n * powf(w, a)
                          - powf(m, a) * powf(w, a * d)))
                    / (powf(m, a * d) * n - n * powf(w, a * d))))
      / n);

  // avoid discontinuous curve by clamping to 0
  c = max((powf(m, a * d) * n * powf(w, a) - powf(m, a) * powf(w, a * d))
          / (powf(m, a * d) * n - n * powf(w, a * d)),
      0.f);
}

std::unique_ptr<LivePixelOp> ToneMapper::attach()
{
  return rkcommon::make_unique<LiveToneMapper>(
      device, exposure, a, b, c, d, acesColor);
}

std::string ToneMapper::toString() const
{
  return "ospray::ToneMapper";
}

LiveToneMapper::LiveToneMapper(api::ISPCDevice &device,
    float exposure,
    float a,
    float b,
    float c,
    float d,
    bool acesColor)
    : AddStructShared(device.getIspcrtContext(), device)
{
  getSh()->super.processPixel =
      reinterpret_cast<ispc::LivePixelOp_processPixel>(
          ispc::LiveToneMapper_processPixel_addr());
  getSh()->exposure = exposure;
  getSh()->a = a;
  getSh()->b = b;
  getSh()->c = c;
  getSh()->d = d;
  getSh()->acesColor = acesColor;
}

} // namespace ospray
