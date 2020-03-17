// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../ImageOp.h"

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

} // namespace ospray
