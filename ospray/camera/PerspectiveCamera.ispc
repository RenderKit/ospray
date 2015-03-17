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

void PerspectiveCamera_initRay(uniform Camera *uniform camera,
                               varying Ray &ray,
                               const varying CameraSample &sample)
{
  uniform PerspectiveCamera *uniform _this = 
    (uniform PerspectiveCamera *uniform)camera;
  const vec3f org = _this->org;
  const vec3f dir = _this->dir_00
    + sample.screen.x * _this->dir_du
    + sample.screen.y * _this->dir_dv;
  setRay(ray,org,normalize(dir));
}

/*! create a new ispc-side version of a perspectivecamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PerspectiveCamera_create(void *uniform cppE)
{
  uniform PerspectiveCamera *uniform cam 
    = uniform new uniform PerspectiveCamera;
  cam->inherited.cppEquivalent = cppE;
  cam->inherited.initRay = PerspectiveCamera_initRay;
  cam->inherited.doesDOF = false;
  return cam;
}

export void PerspectiveCamera_set(void *uniform _camera, 
                                  const uniform vec3f &org,
                                  const uniform vec3f &dir_00,
                                  const uniform vec3f &dir_du,
                                  const uniform vec3f &dir_dv)
{
  uniform PerspectiveCamera *uniform camera
    = (uniform PerspectiveCamera *uniform)_camera;
  camera->org    = org;
  camera->dir_00 = dir_00;
  camera->dir_du = dir_du;
  camera->dir_dv = dir_dv;
}
