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

#include "ospray/ospray.h"

#include "Material.h"
#include "Model.h"

#include "ospcommon/utility/StringManip.h"

namespace ospray {
  namespace sg {

    Material::Material()
    {
      createChild("type", "string", std::string("OBJMaterial"));
      vec3f kd(.7f);
      vec3f ks(.3f);
      createChild("d", "float", 1.f,
                  NodeFlags::required |
                  NodeFlags::gui_color).setMinMax(0.f, 1.f);
      createChild("Kd", "vec3f", kd,
                  NodeFlags::required |
                  NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("Ks", "vec3f", ks,
                  NodeFlags::required |
                  NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("Ns", "float", 10.f,
                  NodeFlags::required |
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

      OSPMaterial mat = nullptr;
      mat = ospNewMaterial2(ctx.ospRendererType.c_str(),
                            child("type").valueAs<std::string>().c_str());

      if (!mat)
      {
        std::cerr << "Warning: Could not create material type '"
                  << type << "'. Replacing with default material." << std::endl;
        static OSPMaterial defaultMaterial = nullptr;
        static OSPRenderer defaultMaterialRenderer = nullptr;
        if (!defaultMaterial || defaultMaterialRenderer != ctx.ospRenderer) {
          defaultMaterial =
            ospNewMaterial2(ctx.ospRendererType.c_str(), "default");

          defaultMaterialRenderer = ctx.ospRenderer;
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

    void Material::postCommit(RenderContext &)
    {
      auto mat = valueAs<OSPMaterial>();

      // handle textures
      for (auto &it : properties.children) {
        auto &child = *it.second;
        if (utility::beginsWith(child.type(), "Texture"))
          ospSetObject(mat, it.first.c_str(), child.valueAs<OSPObject>());
      }
      ospCommit(mat);
    }

    OSP_REGISTER_SG_NODE(Material);
    OSP_REGISTER_SG_NODE(MaterialList);

  } // ::ospray::sg
} // ::ospray
