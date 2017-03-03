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
#include "sg/common/Integrator.h"
#include "ospray/ospray.h"

namespace ospray {
  namespace sg {

    void Material::init()
    {
      add(createNode("type", "string", std::string("OBJMaterial")));
      vec3f kd(10.f/255.f,68.f/255.f,117.f/255.f);
      vec3f ks(208.f/255.f,140.f/255.f,82.f/255.f);
      add(createNode("Kd", "vec3f",kd,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("Kd")->setMinMax(vec3f(0), vec3f(1));
      add(createNode("Ks", "vec3f",ks,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("Ks")->setMinMax(vec3f(0), vec3f(1));
      add(createNode("Ns", "float",10.f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("Ns")->setMinMax(0.f, 100.f);
    }

    void Material::preCommit(RenderContext &ctx)
    {
      assert(ctx.ospRenderer);
      if (ospMaterial != nullptr && ospRenderer == ctx.ospRenderer) return;
      auto mat = ospNewMaterial(ctx.ospRenderer,
                                child("type")->getValue<std::string>().c_str());
      if (!mat)
      {
        std::cerr << "Warning: Could not create material type '"
                  << type << "'. Replacing with default material." << std::endl;
        static OSPMaterial defaultMaterial = nullptr;
        if (!defaultMaterial) {
          defaultMaterial = ospNewMaterial(ctx.integrator->getOSPHandle(), "OBJ");
          vec3f kd(.7f);
          vec3f ks(.3f);
          ospSet3fv(defaultMaterial, "Kd", &kd.x);
          ospSet3fv(defaultMaterial, "Ks", &ks.x);
          ospSet1f(defaultMaterial, "Ns", 99.f);
          ospCommit(defaultMaterial);
        }
        mat = defaultMaterial;
      }

      setValue((OSPObject)mat);
      ospMaterial = mat;
      ospRenderer = ctx.ospRenderer;
    }

    void Material::postCommit(RenderContext &ctx)
    {
      ospCommit(ospMaterial);
    }

    OSP_REGISTER_SG_NODE(Material);

  } // ::ospray::sg
} // ::ospray
