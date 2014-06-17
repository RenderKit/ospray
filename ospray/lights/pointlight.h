#pragma once

#include "light.h"

namespace ospray {

  struct PointLight : public Light {
    PointLight();
    virtual std::string toString() const { return "ospray::PointLight"; }
    virtual void commit();

    vec3f position;               //! world-space position of the light
    vec3f color;                  //! RGB color of the light
    //TODO: Should we bother with range, or just let the lighting code deal with it?
    float range;                  //! range after which light has no effect
  };

}
