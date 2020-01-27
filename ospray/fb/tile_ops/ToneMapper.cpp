// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../ImageOp.h"
#include "ToneMapper_ispc.h"

using namespace ospcommon;

namespace ospray {

/*! \brief Generic tone mapping operator approximating ACES by default. */
struct OSPRAY_SDK_INTERFACE ToneMapper : public TileOp
{
  void commit() override;

  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;

  // Params for the tone mapping curve
  float a, b, c, d;
  bool acesColor;
  float exposure;
};

struct OSPRAY_SDK_INTERFACE LiveToneMapper : public LiveTileOp
{
  LiveToneMapper(FrameBufferView &fbView, void *ispcEquiv);

  ~LiveToneMapper() override;

  void process(Tile &t) override;

  void *ispcEquiv;
};

// Definitions //////////////////////////////////////////////////////////////

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

std::unique_ptr<LiveImageOp> ToneMapper::attach(FrameBufferView &fbView)
{
  void *ispcEquiv = ispc::ToneMapper_create();
  ispc::ToneMapper_set(ispcEquiv, exposure, a, b, c, d, acesColor);
  return ospcommon::make_unique<LiveToneMapper>(fbView, ispcEquiv);
}

std::string ToneMapper::toString() const
{
  return "ospray::ToneMapper";
}

LiveToneMapper::LiveToneMapper(FrameBufferView &_fbView, void *ispcEquiv)
    : LiveTileOp(_fbView), ispcEquiv(ispcEquiv)
{}

LiveToneMapper::~LiveToneMapper()
{
  // TODO WILL: Release the ISPC equiv
}

void LiveToneMapper::process(Tile &tile)
{
  ToneMapper_apply(ispcEquiv, (ispc::Tile &)tile);
}

OSP_REGISTER_IMAGE_OP(ToneMapper, tonemapper);

} // namespace ospray
