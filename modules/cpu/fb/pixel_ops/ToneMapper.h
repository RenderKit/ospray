// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../ImageOp.h"
#include "ToneMapperShared.h"

using namespace rkcommon;

namespace ospray {

/*! \brief Generic tone mapping operator approximating ACES by default. */
struct OSPRAY_SDK_INTERFACE ToneMapper : public PixelOp
{
  void commit() override;

  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;

  // Params for the tone mapping curve
  float a, b, c, d;
  bool acesColor;
  float exposure;
};

struct OSPRAY_SDK_INTERFACE LiveToneMapper
    : public AddStructShared<LivePixelOp, ispc::LiveToneMapper>
{
  LiveToneMapper(FrameBufferView &fbView,
      float exposure,
      float a,
      float b,
      float c,
      float d,
      bool acesColor);
};

} // namespace ospray
