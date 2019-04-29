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
#include "SimpleAO_ispc.h"

namespace ospray {

  struct SimpleAO : public Renderer
  {
    SimpleAO(int defaultNumSamples = 1);
    std::string toString() const override;
    void commit() override;

   private:
    int numSamples{1};
  };

  // SimpleAO definitions /////////////////////////////////////////////////////

  SimpleAO::SimpleAO(int defaultNumSamples) : numSamples(defaultNumSamples)
  {
    setParam<std::string>("externalNameFromAPI", "ao");
    ispcEquivalent = ispc::SimpleAO_create(this);
  }

  std::string SimpleAO::toString() const
  {
    return "ospray::render::SimpleAO";
  }

  void SimpleAO::commit()
  {
    Renderer::commit();
    ispc::SimpleAO_set(getIE(),
                       getParam1i("aoSamples", numSamples),
                       getParam1f("aoRadius", 1e20f),
                       getParam1f("aoIntensity", 1.f));
  }

  OSP_REGISTER_RENDERER(SimpleAO, ao);
  OSP_REGISTER_RENDERER(SimpleAO(1), ao1);
  OSP_REGISTER_RENDERER(SimpleAO(2), ao2);
  OSP_REGISTER_RENDERER(SimpleAO(4), ao4);
  OSP_REGISTER_RENDERER(SimpleAO(8), ao8);
  OSP_REGISTER_RENDERER(SimpleAO(16), ao16);

}  // namespace ospray
