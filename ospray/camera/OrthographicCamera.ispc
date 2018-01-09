// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "OrthographicCamera.ih"

void OrthographicCamera_initRay(uniform Camera *uniform _self,
                                varying Ray &ray,
                                const varying CameraSample &sample)
{
  uniform OrthographicCamera *uniform self = 
    (uniform OrthographicCamera *uniform)_self;
  
  const vec2f screen = Camera_subRegion(_self, sample.screen);

  const vec3f dir = self->dir;
  const vec3f org = self->pos_00
    + screen.x * self->pos_du
    + screen.y * self->pos_dv;

  const float time = Camera_shutterTime(_self, sample.time);
  setRay(ray, org, dir, self->super.nearClip, infinity, time);
}

/*! create a new ispc-side version of a orthographiccamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform OrthographicCamera_create(void *uniform cppE)
{
  uniform OrthographicCamera *uniform cam 
    = uniform new uniform OrthographicCamera;
  cam->super.cppEquivalent = cppE;
  cam->super.initRay = OrthographicCamera_initRay;
  cam->super.doesDOF = false;
  return cam;
}

export void OrthographicCamera_set(void *uniform _self, 
                                   const uniform vec3f &dir,
                                   const uniform vec3f &pos_00,
                                   const uniform vec3f &pos_du,
                                   const uniform vec3f &pos_dv)
{
  uniform OrthographicCamera *uniform self
    = (uniform OrthographicCamera *uniform)_self;
  self->dir = dir;
  self->pos_00 = pos_00;
  self->pos_du = pos_du;
  self->pos_dv = pos_dv;
}
