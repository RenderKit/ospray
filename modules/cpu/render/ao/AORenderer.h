// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Renderer.h"
// ispc shared
#include "AORendererShared.h"

namespace ospray {

struct AORenderer : public AddStructShared<Renderer, ispc::AORenderer>
{
  AORenderer(int defaultAORendererSamples = 1);
  std::string toString() const override;
  void commit() override;

 private:
  int aoSamples{1};
};

} // namespace ospray
