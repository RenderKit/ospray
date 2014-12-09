#pragma once

#include "Light.h"

namespace ospray {

  //!< Base class for spot light objects
  struct SpotLight : public Light {

    //!< Constructor for SpotLight.
    SpotLight();

    //!< Copy understood parameters into class members
    virtual void commit();

    //!< toString is used to aid in printf debugging
    virtual std::string toString() const { return "ospray::SpotLight"; }

    vec3f position;         //!< Position of the SpotLight
    vec3f direction;        //!< Direction that the SpotLight is pointing
    vec3f color;            //!< RGB color of the SpotLight
    float range;            //!< Max influence range of the SpotLight
    float halfAngle;        //!< Half angle of spot light. If angle from intersection to light is greater than this, the light does not influence shading for that intersection
    float angularDropOff;   //!< This gives the drop off of light intensity as angle between intersection point and light position increases

  };

}
