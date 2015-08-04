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

void PTLight__Constructor(uniform PTLight* uniform this,
                        // uniform DestructorFunc destructor,
                        uniform PTLightType type,
                        uniform PTLightTransformFunc transform,
                        uniform ShapeFunc shape,
                        uniform EvalFunc eval,
                        uniform SampleFunc sample)
{
  LOG(print("PTLight__Constructor\n"));
  this->type = type;
  this->transform = transform;
  this->shape = shape;
  this->eval = eval;
  this->sample = sample;
}

void AreaPTLight__Constructor(uniform AreaPTLight* uniform this,
                            // uniform DestructorFunc destructor,
                            uniform PTLightType type,
                            uniform PTLightTransformFunc transform,
                            uniform ShapeFunc shape,
                            uniform EvalFunc eval,
                            uniform SampleFunc sample_,
                            uniform AreaLeFunc Le)
{
  PTLight__Constructor(&this->base,//destructor,
                     type,transform,shape,eval,sample_);
  this->Le = Le;
}

void PTEnvironmentLight__Constructor(uniform PTEnvironmentLight* uniform this,
                                   // uniform DestructorFunc destructor,
                                   uniform PTLightType type,
                                   uniform PTLightTransformFunc transform,
                                   uniform ShapeFunc shape,
                                   uniform EvalFunc eval,
                                   uniform SampleFunc sample_,
                                   uniform EnvironmentLeFunc Le)
{
  PTLight__Constructor(&this->base,// destructor,
                     type,transform,shape,eval,sample_);
  this->Le = Le;
}

export uniform PTLightType PTLight__getType(void* uniform _this)
{
  uniform PTLight *uniform this = (uniform PTLight *uniform) _this;
  return this->type;
}

export void* uniform PTLight__transform(void* uniform _this,
                                      uniform const vec3f& vx, 
                                      uniform const vec3f& vy, 
                                      uniform const vec3f& vz, 
                                      uniform const vec3f& p)
{
  const uniform PTLight *uniform this = (const uniform PTLight *uniform) _this;
  const uniform AffineSpace3f xfm = make_AffineSpace3f(vx,vy,vz,p);
  return this->transform(this,xfm);
}

export void* uniform PTLight__shape(void *uniform _this)
{
  const uniform PTLight *uniform this = (const uniform PTLight *uniform) _this;
  if (!this->shape) return NULL;
  return this->shape(this);
}
