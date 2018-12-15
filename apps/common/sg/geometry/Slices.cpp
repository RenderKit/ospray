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

#include "Slices.h"
#include "../common/Data.h"
#include "../volume/Volume.h"

namespace ospray {
  namespace sg {

    Slices::Slices() : Geometry("slices") {}

    box3f Slices::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      if (hasChild("volume"))
        bbox = child("volume").nodeAs<Volume>()->computeBounds();

      child("bounds") = bbox;

      return bbox;
    }

    void Slices::postRender(RenderContext& ctx)
    {
      auto ospGeometry = valueAs<OSPGeometry>();
      if (ospGeometry && hasChild("volume")) {
        auto ospVolume = child("volume").valueAs<OSPObject>();
        ospSetObject(ospGeometry, "volume", ospVolume);
      }

      if (ospGeometry && hasChild("slices_list")) {
        std::vector<vec4f> slices;

        auto &slices_list = child("slices_list");

        for (auto &child : slices_list.children())
          slices.push_back(child.second->valueAs<vec4f>());

        auto slices_data = ospNewData(slices.size(), OSP_FLOAT4, slices.data());
        ospSetObject(ospGeometry, "planes", slices_data);
        ospRelease(slices_data);
      }

      Geometry::postRender(ctx);
    }

    void Slices::preTraverse(RenderContext &ctx,
                             const std::string& operation,
                             bool& traverseChildren)
    {
      traverseChildren = operation != "render";
      Node::preTraverse(ctx,operation, traverseChildren);
      if (operation == "render")
        preRender(ctx);
    }

    OSP_REGISTER_SG_NODE(Slices);

  }// ::ospray::sg
}// ::ospray
