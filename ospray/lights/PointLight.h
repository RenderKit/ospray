#pragma once

#include "Light.h"

namespace ospray {

  //! Base class for PointLight objects, inherits from Light
  struct PointLight : public Light {
    public:

      //!Construct a new point light.
      PointLight();

      //! toString is used to aid in printf debugging
      virtual std::string toString() const { return "ospray::PointLight"; }

      //! Copy understood parameters into member parameters
      virtual void commit();

      vec3f position;               //! world-space position of the light
      vec3f color;                  //! RGB color of the light
      float range;                  //! range after which light has no effect

  };

}
