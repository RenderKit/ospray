// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../Renderer.h"

namespace ospray {

struct SciVis : public Renderer
{
  SciVis(int defaultAOSamples = 0, bool defaultShadowsEnabled = false);
  std::string toString() const override;
  void commit() override;
  void *beginFrame(FrameBuffer *, World *) override;

 private:
  int aoSamples{0};
  bool shadowsEnabled{false};
};

} // namespace ospray
