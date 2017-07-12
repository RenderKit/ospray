// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "Geometry.h"
#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    Geometry::Geometry(const std::string &type)
    {
      createChild("material", "Material");
      createChild("type", "string", type);
      setValue((OSPObject)nullptr);
    }

    std::string Geometry::toString() const
    {
      return "ospray::sg::Geometry";
    }

    void Geometry::preCommit(RenderContext &ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (!ospGeometry) {
        auto type = child("type").valueAs<std::string>();
        ospGeometry = ospNewGeometry(type.c_str());
        setValue((OSPObject)ospGeometry);

        child("bounds").setValue(computeBounds());
      }
    }

    void Geometry::postCommit(RenderContext &ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (hasChild("material")) {
        ospSetMaterial(ospGeometry,
                       (OSPMaterial)child("material").valueAs<OSPObject>());
      }
      ospCommit(ospGeometry);
    }

    void Geometry::postRender(RenderContext& ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (ospGeometry)
        ospAddGeometry(ctx.currentOSPModel, ospGeometry);
    }

  } // ::ospray::sg
} // ::ospray
