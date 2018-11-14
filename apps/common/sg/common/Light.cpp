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

#include "Light.h"

namespace ospray {
  namespace sg {

    Light::Light()
    {
      setValue((OSPLight)nullptr);
    }

    Light::Light(const std::string &type) : Light()
    {
      this->type = type;
    }

    void Light::preCommit(RenderContext &)
    {
      if (!valueAs<OSPLight>())
        setValue(ospNewLight3(type.c_str()));
    }

    void Light::postCommit(RenderContext &)
    {
      ospCommit(valueAs<OSPLight>());
    }

    std::string Light::toString() const
    {
      return "ospray::sg::Light<" + type + ">";
    }

    AmbientLight::AmbientLight()
      : Light("AmbientLight")
    {
      createChild("color", "vec3f", vec3f(.7f,.8f,1.f),
                      NodeFlags::required |
                      NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));
      createChild("intensity", "float", 0.2f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,12.f);
    }

    DirectionalLight::DirectionalLight()
      : Light("DirectionalLight")
    {
      createChild("direction", "vec3f", vec3f(-.3,.2,.4),
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(vec3f(-1), vec3f(1));

      createChild("color", "vec3f", vec3f(1.f),
                      NodeFlags::required |
                      NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));

      createChild("intensity", "float", 3.f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,12.f);

      createChild("angularDiameter", "float", 0.53f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,4.f);
    }

    PointLight::PointLight()
      : Light("PointLight")
    {
      createChild("color", "vec3f", vec3f(1.f),
                      NodeFlags::required |
                      NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));

      createChild("position", "vec3f", vec3f(0.f),
                      NodeFlags::required);

      createChild("intensity", "float", 3.f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,999.f);

      createChild("radius", "float", 0.0f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,4.f);
    }

    QuadLight::QuadLight()
      : Light("QuadLight")
    {
      createChild("color", "vec3f", vec3f(1.f),
                      NodeFlags::required |
                      NodeFlags::gui_color).setMinMax(vec3f(0), vec3f(1));

      createChild("intensity", "float", 1.f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(0.f,999.f);

      createChild("position", "vec3f", vec3f(0.f),
                      NodeFlags::required);

      createChild("edge1", "vec3f", vec3f(0.f, 1.f, 0.f),
                      NodeFlags::required);

      createChild("edge2", "vec3f", vec3f(0.f, 0.f, 1.f),
                      NodeFlags::required);
    }

    HDRILight::HDRILight()
      : Light("hdri")
    {
      createChild("up", "vec3f", vec3f(0.f,1.f,0.f),
                NodeFlags::required).setMinMax(vec3f(-1), vec3f(1));
      createChild("dir", "vec3f", vec3f(1.f,0.f,0.f),
                NodeFlags::required).setMinMax(vec3f(-1), vec3f(1));
      createChild("intensity", "float", 1.f,
                NodeFlags::required |
                NodeFlags::gui_slider).setMinMax(0.f,12.f);
    }

    void HDRILight::postCommit(RenderContext &ctx)
    {
      if (hasChild("map")) {
        ospSetObject(valueAs<OSPObject>(), "map",
          child("map").valueAs<OSPObject>());
      }

      Light::postCommit(ctx);
    }

    bool HDRILight::computeValid()
    {
      if (hasChild("map") && child("map").valueAs<OSPObject>() != nullptr)
        return Node::computeValid();
      else
        return false;
    }


    OSP_REGISTER_SG_NODE(Light);
    OSP_REGISTER_SG_NODE(DirectionalLight);
    OSP_REGISTER_SG_NODE(AmbientLight);
    OSP_REGISTER_SG_NODE(PointLight);
    OSP_REGISTER_SG_NODE(QuadLight);
    OSP_REGISTER_SG_NODE(HDRILight);

  } // ::ospray::sg
} // ::ospray

