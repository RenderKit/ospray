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

#include "sg/common/Light.h"

namespace ospray {
  namespace sg {

    void Light::preCommit(RenderContext &ctx)
    {
      if (!ospLight)
        ospLight = ospNewLight(ctx.ospRenderer, type.c_str());
      ospCommit(ospLight);
      setValue((OSPObject)ospLight);
    }

    void Light::postCommit(RenderContext &ctx)
    {
      ospCommit(ospLight);
    }

    std::string Light::toString() const
    {
      return "ospray::sg::Light";
    }

    void ospray::sg::AmbientLight::init()
    {
      add(createNode("color", "vec3f", vec3f(.7f,.8f,1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color").setMinMax(vec3f(0), vec3f(1));
      add(createNode("intensity", "float", 0.2f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity").setMinMax(0.f,4.f);
    }

    void ospray::sg::DirectionalLight::init()
    {
      add(createNode("direction", "vec3f", vec3f(-.3,.2,.4),
                     NodeFlags::required | NodeFlags::gui_slider));
      child("direction").setMinMax(vec3f(-1), vec3f(1));
      add(createNode("color", "vec3f", vec3f(1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color").setMinMax(vec3f(0), vec3f(1));
      add(createNode("intensity", "float", 3.f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity").setMinMax(0.f,4.f);
      add(createNode("angularDiameter", "float", 0.01f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("angularDiameter").setMinMax(0.f,4.f);
    }

    void ospray::sg::PointLight::init()
    {
      add(createNode("color", "vec3f", vec3f(1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color").setMinMax(vec3f(0), vec3f(1));
      add(createNode("position", "vec3f", vec3f(0.f),
                     NodeFlags::required | NodeFlags::valid_min_max));
      add(createNode("intensity", "float", 3.f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity").setMinMax(0.f,4.f);
      add(createNode("radius", "float", 1.0f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("radius").setMinMax(0.f,4.f);
    }

    OSP_REGISTER_SG_NODE(Light);
    OSP_REGISTER_SG_NODE(DirectionalLight);
    OSP_REGISTER_SG_NODE(AmbientLight);
    OSP_REGISTER_SG_NODE(PointLight);

  } // ::ospray::sg
} // ::ospray

