// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
                     getParam1i("aoSamples", aoSamples),
                     getParam1f("aoRadius", 1e20f),
                     getParam1f("aoIntensity", 1.f));
  }

  OSP_REGISTER_RENDERER(SciVis, scivis);
  OSP_REGISTER_RENDERER(SciVis, sv);

  OSP_REGISTER_RENDERER(SciVis, ao);
  OSP_REGISTER_RENDERER(SciVis(1), ao1);
  OSP_REGISTER_RENDERER(SciVis(2), ao2);
  OSP_REGISTER_RENDERER(SciVis(4), ao4);
  OSP_REGISTER_RENDERER(SciVis(8), ao8);
  OSP_REGISTER_RENDERER(SciVis(16), ao16);

}  // namespace ospray
