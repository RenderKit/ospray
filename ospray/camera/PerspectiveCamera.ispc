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

#include "PerspectiveCamera.ih"

void PerspectiveCamera_initRay(uniform Camera *uniform _self,
                               varying Ray &ray,
                               const varying CameraSample &sample)
{
  uniform PerspectiveCamera *uniform self = 
    (uniform PerspectiveCamera *uniform)_self;

  const vec3f org = self->org;
  const vec3f dir = self->dir_00
    + sample.screen.x * self->dir_du
    + sample.screen.y * self->dir_dv;

  setRay(ray,org,normalize(dir),self->super.nearClip,infinity);
}

/*! create a new ispc-side version of a perspectivecamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PerspectiveCamera_create(void *uniform cppE)
{
  uniform PerspectiveCamera *uniform cam 
    = uniform new uniform PerspectiveCamera;
  cam->super.cppEquivalent = cppE;
  cam->super.initRay = PerspectiveCamera_initRay;
  cam->super.doesDOF = false;
  return cam;
}

export void PerspectiveCamera_set(void *uniform _self, 
                                  const uniform vec3f &org,
                                  const uniform vec3f &dir_00,
                                  const uniform vec3f &dir_du,
                                  const uniform vec3f &dir_dv,
                                  const uniform float nearClip)
{
  uniform PerspectiveCamera *uniform self
    = (uniform PerspectiveCamera *uniform)_self;
  self->org    = org;
  self->dir_00 = dir_00;
  self->dir_du = dir_du;
  self->dir_dv = dir_dv;
  self->super.nearClip = nearClip;
}
