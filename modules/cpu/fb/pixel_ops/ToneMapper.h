// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/PixelOp.h"
// ispc shared
#include "ToneMapperShared.h"

using namespace rkcommon;

namespace ospray {

/*! \brief Generic tone mapping operator approximating ACES by default. */
struct OSPRAY_SDK_INTERFACE ToneMapper : public PixelOp
{
  ToneMapper(api::Device &device)
      : PixelOp(static_cast<api::ISPCDevice &>(device))
  {}

  void commit() override;

  std::unique_ptr<LivePixelOp> attach() override;

  std::string toString() const override;

  // Params for the tone mapping curve
  float a, b, c, d;
  bool acesColor;
  float exposure;
};

struct OSPRAY_SDK_INTERFACE LiveToneMapper
    : public AddStructShared<LivePixelOp, ispc::LiveToneMapper>
{
  LiveToneMapper(api::ISPCDevice &device,
      float exposure,
      float a,
      float b,
      float c,
      float d,
      bool acesColor);
};

} // namespace ospray
