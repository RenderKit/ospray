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

#include "PTLight.ih"
#include "../samplers/ShapeSampler.ih"

struct PTAmbientLight
{
  PTEnvironmentLight base;
  vec3f L;
};

uniform PTEnvironmentLight* uniform PTAmbientLight__New(const uniform vec3f& L);

uniform PTLight* uniform PTAmbientLight__transform(const uniform PTLight *uniform _this, 
                                               const uniform AffineSpace3f& xfm) 
{
  const uniform PTAmbientLight* uniform this = (const uniform PTAmbientLight* uniform)_this;
  return (uniform PTLight* uniform) PTAmbientLight__New(this->L);
}

varying vec3f PTAmbientLight__Le(const uniform PTEnvironmentLight* uniform _this, varying const vec3f& wo) 
{
  const uniform PTAmbientLight* uniform this = (const uniform PTAmbientLight* uniform)_this;
  return this->L;
}

varying vec3f PTAmbientLight__eval(const uniform PTLight* uniform _this, 
                                 varying const DifferentialGeometry& dg, 
                                 varying const vec3f& wi) 
{
  const uniform PTAmbientLight* uniform this = (const uniform PTAmbientLight* uniform)_this;
  return this->L;
}

varying vec3f PTAmbientLight__sample(const uniform PTLight* uniform _this,
                                   varying const DifferentialGeometry& dg, 
                                   varying Sample3f& wi,
                                   varying float& tMax,
                                   varying const vec2f& s) 
{
  const uniform PTAmbientLight* uniform this = (const uniform PTAmbientLight* uniform)_this;
  wi = cosineSampleHemisphere(//.5f,.5f,
                              s.x, s.y, 
                              dg.Ns);
  tMax = 1e20f;
  return this->L;
}

void PTAmbientLight__Constructor(uniform PTAmbientLight* uniform this, const uniform vec3f L) 
{
  PTEnvironmentLight__Constructor(&this->base,//Light__Destructor,
                                ENV_LIGHT,
                                PTAmbientLight__transform,NULL,PTAmbientLight__eval,PTAmbientLight__sample,
                                PTAmbientLight__Le);
  this->L = L;
}

uniform PTEnvironmentLight* uniform PTAmbientLight__New(const uniform vec3f& L)
{
  uniform PTAmbientLight* uniform light = uniform new uniform PTAmbientLight;
  PTAmbientLight__Constructor(light,L);
  return (PTEnvironmentLight*)light;
}

export void* uniform PTAmbientLight__new(const uniform vec3f& L) {
  return PTAmbientLight__New(L);
}
