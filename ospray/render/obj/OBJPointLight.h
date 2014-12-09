#pragma once

#include "ospray/lights/PointLight.h"

namespace ospray {
  namespace obj {

    //! OBJ renderer specific implementation of PointLights
    struct OBJPointLight : public PointLight {
      OBJPointLight();
      //! \brief common function to help printf-debugging
      virtual std::string toString() const { return "ospray::objrenderer::OBJPointLight"; }

      //! \brief commit the light's parameters
      virtual void commit();

      //! destructor, to clean up
      virtual ~OBJPointLight();

      //Attenuation will be calculated as 1/( constantAttenuation + linearAttenuation * distance + quadraticAttenuation * distance * distance )
      float constantAttenuation;    //! Constant light attenuation
      float linearAttenuation;      //! Linear light attenuation
      float quadraticAttenuation;   //! Quadratic light attenuation
    };

  } // ::ospray::obj
} // ::ospray
