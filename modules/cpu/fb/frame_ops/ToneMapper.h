// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOp.h"
// ispc shared
#include "ToneMapperShared.h"

namespace ospray {

/*! \brief Generic tone mapping operator approximating ACES by default. */
struct OSPRAY_SDK_INTERFACE ToneMapperFrameOp : public FrameOp
{
  ToneMapperFrameOp(api::Device &device);

  void commit() override;

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;

 private:
  // Params for the tone mapping curve
  float a, b, c, d;
  bool acesColor;
  float exposure;
};

struct OSPRAY_SDK_INTERFACE LiveToneMapperFrameOp
    : public AddStructShared<LiveFrameOp, ispc::LiveToneMapper>
{
  LiveToneMapperFrameOp(api::ISPCDevice &device,
      FrameBufferView &fbView,
      float exposure,
      float a,
      float b,
      float c,
      float d,
      bool acesColor);

  // Execute FrameOp kernel
  void process(void *waitEvent) override;
};

} // namespace ospray
