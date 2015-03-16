// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "AmbientLight.ih"

//!< Construct a new ispc-side AmbientLight object
extern void AmbientLight_Constructor(uniform AmbientLight *uniform THIS,
                                     void *uniform cppEquivalent,
                                     const uniform vec3f &color,
                                     const uniform float &intensity)
{
  Light_Constructor(&THIS->base, cppEquivalent);
  THIS->color = color;
  THIS->intensity = intensity;
  THIS->base.computeRadiance = AmbientLight_computeRadiance;
}

//!< Destroy an ispc-side DirectionLight object
extern void AmbientLight_Destructor(uniform AmbientLight *uniform THIS)
{
  Light_Destructor(&THIS->base);
}

//! Compute the radiance at a point from a sample on the light sans occluders, return the extant light vector and distance.
varying vec3f AmbientLight_computeRadiance(void *uniform pointer, 
                                           const varying vec3f &coordinates, 
                                           varying vec3f &direction, 
                                           varying float &distance) 
{
  //!Since its an ambient light postion and distance and direction dont matter.
  uniform AmbientLight *uniform THIS = (uniform AmbientLight *uniform)pointer;
  return THIS->intensity * THIS->color;

}

//!< Create an ispc_side AmbientLight object (called from c++)
export void *uniform AmbientLight_create(void *uniform cppEquivalent)
{
  uniform AmbientLight *uniform light = uniform new uniform AmbientLight;
  AmbientLight_Constructor(light, cppEquivalent, make_vec3f(1.f, 1.f, 1.f), 1.f);
  return light;
}

//!< Destroy an ispc-side AmbientLIght object (called from c++)
export void AmbientLight_destroy(void *uniform _light) {
  uniform AmbientLight *uniform light = uniform new uniform AmbientLight;
  AmbientLight_Destructor(light);
  delete light;
}

//!< Set the parameters of an ispc-side AmbientLight object (called from c++ side)
export void AmbientLight_set( void *uniform _light,
                              const uniform vec3f &color,
                              const uniform float &intensity)
{
  uniform AmbientLight *uniform light = (uniform AmbientLight *uniform)_light;
  light->color = color;
  light->intensity = intensity;
}
