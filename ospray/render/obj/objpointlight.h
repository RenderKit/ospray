#pragma once

#include "ospray/lights/pointlight.h"

namespace ospray {
  namespace obj {
    struct OBJPointLight : public PointLight {
      //! \brief common function to help printf-debugging
      virtual std::string toString() const { return "ospray::objrenderer::OBJPointLight"; }

      //! \brief commit the light's parameters
      virtual void commit();

      //! destructor, to clean up
      virtual ~OBJPointLight();

      float constantAttenuation;
      float linearAttenuation;
      float quadraticAttenuation;
    };
  }
}
