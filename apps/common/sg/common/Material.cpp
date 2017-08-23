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

#include "sg/common/Material.h"
#include "sg/common/World.h"
#include "ospray/ospray.h"

namespace ospray {
  namespace sg {

    Material::Material()
    {
      createChild("type", "string", std::string("OBJMaterial"));
      vec3f kd(.7f);
      vec3f ks(.3f);
      createChild("d", "float", 1.f,
                  NodeFlags::required |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_color).setMinMax(0.f, 1.f);
      createChild("Ka", "vec3f", vec3f(0),
                  NodeFlags::required |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("Kd", "vec3f", kd,
                  NodeFlags::required |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("Ks", "vec3f", ks,
                  NodeFlags::required |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("Ns", "float", 10.f,
                  NodeFlags::required |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_slider).setMinMax(2.f, 1000.f);
      setValue((OSPMaterial)nullptr);
    }

    std::string Material::toString() const
    {
      return "ospray::viewer::sg::Material";
    }

    void Material::preCommit(RenderContext &ctx)
    {
      assert(ctx.ospRenderer);
      const bool typeChanged =
        child("type").lastModified() > child("type").lastCommitted();

      if (!typeChanged && valueAs<OSPMaterial>() != nullptr
          && ospRenderer == ctx.ospRenderer)
      {
        return;
      }

      auto mat = ospNewMaterial(ctx.ospRenderer,
                                child("type").valueAs<std::string>().c_str());
      if (!mat)
      {
        std::cerr << "Warning: Could not create material type '"
                  << type << "'. Replacing with default material." << std::endl;
        static OSPMaterial defaultMaterial = nullptr;
        if (!defaultMaterial) {
          defaultMaterial = ospNewMaterial(ctx.ospRenderer, "OBJ");
          const float kd[] = {.7f, .7f, .7f};
          const float ks[] = {.3f, .3f, .3f};
          ospSet3fv(defaultMaterial, "Kd", kd);
          ospSet3fv(defaultMaterial, "Ks", ks);
          ospSet1f(defaultMaterial, "Ns", 10.f);
          ospCommit(defaultMaterial);
        }
        mat = defaultMaterial;
      }

      setValue(mat);
      ospRenderer = ctx.ospRenderer;
    }

    void Material::postCommit(RenderContext &ctx)
    {
      if (hasChild("map_Kd"))
        ospSetObject(valueAs<OSPMaterial>(), "map_Kd",
          child("map_Kd").valueAs<OSPObject>());
      if (hasChild("map_Ks"))
        ospSetObject(valueAs<OSPMaterial>(), "map_Ks",
          child("map_Ks").valueAs<OSPObject>());
      if (hasChild("map_Ns"))
        ospSetObject(valueAs<OSPMaterial>(), "map_Ns",
          child("map_Ns").valueAs<OSPObject>());
      if (hasChild("map_d"))
        ospSetObject(valueAs<OSPMaterial>(), "map_d",
          child("map_d").valueAs<OSPObject>());
      if (hasChild("map_Bump"))
        ospSetObject(valueAs<OSPMaterial>(), "map_Bump",
          child("map_Bump").valueAs<OSPObject>());

      ospCommit(valueAs<OSPMaterial>());
    }

    OSP_REGISTER_SG_NODE(Material);

  } // ::ospray::sg
} // ::ospray
