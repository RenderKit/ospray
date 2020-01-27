// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../Renderer.h"
// ispc exports
#include "SciVis_ispc.h"

namespace ospray {

struct SciVis : public Renderer
{
  SciVis(int defaultAOSamples = 1);
  std::string toString() const override;
  void commit() override;

 private:
  int aoSamples{1};
};

// SciVis definitions /////////////////////////////////////////////////////

SciVis::SciVis(int defaultNumSamples) : aoSamples(defaultNumSamples)
{
  ispcEquivalent = ispc::SciVis_create(this);
}

std::string SciVis::toString() const
{
  return "ospray::render::SciVis";
}

void SciVis::commit()
{
  Renderer::commit();

  ispc::SciVis_set(getIE(),
      getParam<int>("aoSamples", aoSamples),
      getParam<float>("aoRadius", 1e20f),
      getParam<float>("aoIntensity", 1.f),
      getParam<float>("volumeSamplingRate", 1.f));
}

OSP_REGISTER_RENDERER(SciVis, scivis);
OSP_REGISTER_RENDERER(SciVis, ao);

} // namespace ospray
