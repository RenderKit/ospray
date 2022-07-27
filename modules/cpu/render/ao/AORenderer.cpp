// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "render/ao/AORenderer_ispc.h"

namespace ospray {

AORenderer::AORenderer(int defaultNumSamples) : aoSamples(defaultNumSamples)
{
  getSh()->super.renderSample = ispc::AORenderer_renderSample_addr();
}

std::string AORenderer::toString() const
{
  return "ospray::render::AO";
}

void AORenderer::commit()
{
  Renderer::commit();

  getSh()->aoSamples = getParam<int>("aoSamples", aoSamples);
  getSh()->aoRadius =
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f));
  getSh()->aoIntensity = getParam<float>("aoIntensity", 1.f);
  getSh()->volumeSamplingRate = getParam<float>("volumeSamplingRate", 1.f);
}

} // namespace ospray
