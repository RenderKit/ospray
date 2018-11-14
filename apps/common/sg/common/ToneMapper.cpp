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

#include "ToneMapper.h"

namespace ospray {
  namespace sg {

    ToneMapper::ToneMapper()
    {
      setValue(ospNewPixelOp("tonemapper"));

      createChild("enabled", "bool", true);

      createChild("exposure", "float", 0.0f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(-8.f, 8.f);

      createChild("contrast", "float", 1.6773f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(1.f, 5.f);

      createChild("shoulder", "float", 0.9714f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.9f, 1.f);

      createChild("midIn", "float", 0.18f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.f, 1.f);

      createChild("midOut", "float", 0.18f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.f, 1.f);

      createChild("hdrMax", "float", 11.0785f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(1.f, 64.f);

      createChild("acesColor", "bool", true);
    }

    void ToneMapper::preTraverse(RenderContext &,
        const std::string& operation,
        bool& traverseChildren)
    {
      if (operation != "commit")
        return;

      if (!child("enabled").valueAs<bool>())
        traverseChildren = false;
    }

    void ToneMapper::postCommit(RenderContext &)
    {
      if (!child("enabled").valueAs<bool>())
        return;

      auto toneMapper = valueAs<OSPPixelOp>();

      float exposure = child("exposure").valueAs<float>();
      float linearExposure = exp2(exposure);
      ospSet1f(toneMapper, "exposure", linearExposure);

      ospCommit(toneMapper);
    }

    std::string ToneMapper::toString() const
    {
      return "ospray::sg::ToneMapper";
    }

    OSP_REGISTER_SG_NODE(ToneMapper);

  } // ::ospray::sg
} // ::ospray
