// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "TransferFunction.h"

namespace ospray {
  namespace sg {

    TransferFunction::TransferFunction()
    {
      setValue((OSPTransferFunction)nullptr);
      createChild("valueRange", "vec2f", vec2f(0.f, 1.f));
      createChild("numSamples", "int", 256);

      auto colors = createChild("colors",
                                "DataVector3f").nodeAs<DataVector3f>();

      // 'Jet' transfer function
      colors->v.emplace_back(0       , 0, 0.562493);
      colors->v.emplace_back(0       , 0, 1       );
      colors->v.emplace_back(0       , 1, 1       );
      colors->v.emplace_back(0.500008, 1, 0.500008);
      colors->v.emplace_back(1       , 1, 0       );
      colors->v.emplace_back(1       , 0, 0       );
      colors->v.emplace_back(0.500008, 0, 0       );

      auto alpha = createChild("alpha", "DataVector2f").nodeAs<DataVector2f>();
      alpha->v.emplace_back(0.f, 0.f);
      alpha->v.emplace_back(1.f, 1.f);

      createChild("opacities", "DataVector1f");
    }

    float TransferFunction::interpolatedAlpha(const DataBuffer &alpha,
                                              float x)
    {
      auto firstAlpha = alpha.get<vec2f>(0);
      if (x <= firstAlpha.x)
        return firstAlpha.y;

      for (uint32_t i = 1; i < alpha.size(); i++) {
        auto current  = alpha.get<vec2f>(i);
        auto previous = alpha.get<vec2f>(i - 1);
        if (x <= current.x) {
          const float t = (x - previous.x) / (current.x - previous.x);
          return (1.0 - t) * previous.y + t * current.y;
        }
      }

      auto lastAlpha = alpha.get<vec2f>(alpha.size() - 1);
      return lastAlpha.y;
    }

    void TransferFunction::calculateOpacities()
    {
      auto numSamples = child("numSamples").valueAs<int>();

      float x0 = 0.f;
      float dx = (1.f - x0) / (numSamples-1);

      auto alpha     = child("alpha").nodeAs<DataBuffer>();
      auto opacities = child("opacities").nodeAs<DataVector1f>();

      opacities->v.clear();
      for (int i = 0; i < numSamples; i++)
        opacities->push_back(interpolatedAlpha(*alpha, i * dx));
    }

    void TransferFunction::preCommit(RenderContext &)
    {
      auto ospTransferFunction = valueAs<OSPTransferFunction>();
      if (!ospTransferFunction) {
        ospTransferFunction = ospNewTransferFunction("piecewise_linear");
        setValue(ospTransferFunction);
      }

      calculateOpacities();
    }

    void TransferFunction::postCommit(RenderContext &)
    {
      ospCommit(valueAs<OSPTransferFunction>());
    }

    std::string TransferFunction::toString() const
    {
      return "ospray::sg::TransferFunction";
    }

    OSP_REGISTER_SG_NODE(TransferFunction);

  } // ::ospray::sg
} // ::ospray
