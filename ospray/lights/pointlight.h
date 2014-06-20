#pragma once

#include "light.h"

namespace ospray {

  struct PointLight : public Light {
    protected:
      PointLight();
    public:
      virtual std::string toString() const { return "ospray::PointLight"; }
      virtual void commit();

      vec3f position;               //! world-space position of the light
      vec3f color;                  //! RGB color of the light
      float range;                  //! range after which light has no effect
  };

}
