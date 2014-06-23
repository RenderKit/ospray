#pragma once

#include "ospray/lights/light.h"
#include "ospray/common/managed.h"

namespace ospray {

  /*! Base class for DirectionalLight objects, inherits from Light 
   *  Used for simulating a point light that is infinitely distant and projects parallel rays of light
   *  across the entire scene */
  struct DirectionalLight : public Light {
    //!< Construct a new DirectionalLight object
    DirectionalLight();

    //!< toString is used to aid in printf debugging
    virtual std::string toString() const { return "ospray::DirectionalLight"; }

    //!< Copy understood parameters into member parameters
    virtual void commit();

    vec3f color;          //!< RGB color of the emitted light
    vec3f direction;      //!< Direction of the emitted rays
  };

}
