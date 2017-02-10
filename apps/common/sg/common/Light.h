// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#pragma once

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"
#include "sg/camera/Camera.h"

#include <cfloat>

namespace ospray {
  namespace sg {

/*! a light node - the generic light node */
struct Light : public sg::Node {
  //! \brief constructor
  Light() : type("none"), ospLight(nullptr) {};
  Light(const std::string &type) : type(type), ospLight(nullptr) {};
  
  virtual void preCommit(RenderContext &ctx)
  {
  	if (!ospLight)
  	  ospLight = ospNewLight(ctx.ospRenderer, type.c_str());
  	ospCommit(ospLight);
  	setValue((OSPObject)ospLight);
  }

  virtual void postCommit(RenderContext &ctx)
  {
    ospCommit(ospLight);
  }

  //! \brief returns a std::string with the c++ name of this class 
  virtual    std::string toString() const override { return "ospray::sg::Light"; }

  /*! \brief light type, i.e., 'DirectionalLight', 'PointLight', ... */
  const std::string type;
  OSPLight ospLight;
};

/*! a light node - the generic light node */
struct AmbientLight : public Light {
  //! \brief constructor
  AmbientLight() : Light("AmbientLight") {}

  virtual void init() override
  { 
  	add(createNode("color", "vec3f", vec3f(.7f,.8f,1.f),NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
    getChild("color")->setMinMax(vec3f(0), vec3f(1));
  	add(createNode("intensity", "float", 0.2f,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
    getChild("intensity")->setMinMax(0.f,4.f);
  }
};

/*! a light node - the generic light node */
struct DirectionalLight : public Light {
  //! \brief constructor
  DirectionalLight() : Light("DirectionalLight") {}

  virtual void init() override
  { 
  	add(createNode("direction", "vec3f", vec3f(-.3,.2,.4), NodeFlags::required | NodeFlags::gui_slider));
    getChild("direction")->setMinMax(vec3f(-1), vec3f(1));
  	add(createNode("color", "vec3f", vec3f(1.f),NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
    getChild("color")->setMinMax(vec3f(0), vec3f(1));
  	add(createNode("intensity", "float", 3.f,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
    getChild("intensity")->setMinMax(0.f,4.f);
  	add(createNode("angularDiameter", "float", 0.01f,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
    getChild("angularDiameter")->setMinMax(0.f,4.f);
  }
};

/*! a light node - the generic light node */
struct PointLight : public Light {
  //! \brief constructor
  PointLight() : Light("PointLight") {}

  virtual void init() override
  { 
    add(createNode("color", "vec3f", vec3f(1.f),NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
    getChild("color")->setMinMax(vec3f(0), vec3f(1));
    add(createNode("position", "vec3f", vec3f(0.f),NodeFlags::required | NodeFlags::valid_min_max));
    add(createNode("intensity", "float", 3.f,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
    getChild("intensity")->setMinMax(0.f,4.f);
    add(createNode("radius", "float", 1.0f,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
    getChild("radius")->setMinMax(0.f,4.f);
  }
};

}
}