// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "PanoramicCamera.ih"
#include "math/sampling.ih"

void PanoramicCamera_initRay(uniform Camera *uniform _self,
                             varying Ray &ray,
                             const varying CameraSample &sample)
{
  uniform PanoramicCamera *uniform self = (uniform PanoramicCamera *uniform)_self;

  const vec3f org = self->pos;

  const float phi = two_pi * sample.screen.x;
  const float theta = -M_PI * sample.screen.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  // transform into camera frame
  const vec3f dir = self->frame * localDir;

  setRay(ray, org, dir, self->super.nearClip, infinity);
}

/*! create a new ispc-side version of a panoramiccamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PanoramicCamera_create(void *uniform cppE)
{
  uniform PanoramicCamera *uniform cam = uniform new uniform PanoramicCamera;
  cam->super.cppEquivalent = cppE;
  cam->super.initRay = PanoramicCamera_initRay;
  cam->super.doesDOF = false;
  return cam;
}

export void PanoramicCamera_set(void *uniform _self, 
                                const uniform vec3f &pos,
                                const uniform linear3f &frame,
                                const uniform float nearClip)
{
  uniform PanoramicCamera *uniform self = (uniform PanoramicCamera *uniform)_self;
  self->pos = pos;
  self->frame = frame;
  self->super.nearClip = nearClip;
}
