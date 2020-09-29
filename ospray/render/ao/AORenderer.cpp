// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "render/ao/AORenderer_ispc.h"

namespace ospray {

AORenderer::AORenderer(int defaultNumSamples) : aoSamples(defaultNumSamples)
{
  ispcEquivalent = ispc::AORenderer_create(this);
}

std::string AORenderer::toString() const
{
  return "ospray::render::AO";
}

void AORenderer::commit()
{
  Renderer::commit();

  ispc::AORenderer_set(getIE(),
      getParam<int>("aoSamples", aoSamples),
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f)),
      getParam<float>("aoIntensity", 1.f),
      getParam<float>("volumeSamplingRate", 1.f));
}

} // namespace ospray
