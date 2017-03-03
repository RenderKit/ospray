#include "sg/common/Light.h"

namespace ospray {
  namespace sg {

    void ospray::sg::AmbientLight::init()
    {
      add(createNode("color", "vec3f", vec3f(.7f,.8f,1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color")->setMinMax(vec3f(0), vec3f(1));
      add(createNode("intensity", "float", 0.2f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity")->setMinMax(0.f,4.f);
    }

    void ospray::sg::DirectionalLight::init()
    {
      add(createNode("direction", "vec3f", vec3f(-.3,.2,.4),
                     NodeFlags::required | NodeFlags::gui_slider));
      child("direction")->setMinMax(vec3f(-1), vec3f(1));
      add(createNode("color", "vec3f", vec3f(1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color")->setMinMax(vec3f(0), vec3f(1));
      add(createNode("intensity", "float", 3.f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity")->setMinMax(0.f,4.f);
      add(createNode("angularDiameter", "float", 0.01f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("angularDiameter")->setMinMax(0.f,4.f);
    }

    void ospray::sg::PointLight::init()
    {
      add(createNode("color", "vec3f", vec3f(1.f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));
      child("color")->setMinMax(vec3f(0), vec3f(1));
      add(createNode("position", "vec3f", vec3f(0.f),
                     NodeFlags::required | NodeFlags::valid_min_max));
      add(createNode("intensity", "float", 3.f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("intensity")->setMinMax(0.f,4.f);
      add(createNode("radius", "float", 1.0f,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("radius")->setMinMax(0.f,4.f);
    }

    OSP_REGISTER_SG_NODE(Light);
    OSP_REGISTER_SG_NODE(DirectionalLight);
    OSP_REGISTER_SG_NODE(AmbientLight);
    OSP_REGISTER_SG_NODE(PointLight);

  } // ::ospray::sg
} // ::ospray

