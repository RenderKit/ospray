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

#include "Camera.ih"
#include "math/sampling.ih"

void PanoramicCamera_initRay(Camera *uniform self,
                             varying Ray &ray,
                             const varying CameraSample &sample)
{
  const vec2f screen = Camera_subRegion(self, sample.screen);

  const float phi = two_pi * screen.x;
  const float theta = M_PI * screen.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  // trnasform to camera- and then to world-space
  const vec3f dir = self->frame *
    make_vec3f(-localDir.y, -localDir.z, localDir.x);

  const float time = Camera_shutterTime(self, sample.time);
  const vec3f org = self->org;
  setRay(ray, org, dir, self->nearClip, infinity, time);
}

/*! create a new ispc-side version of a panoramiccamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PanoramicCamera_create(void *uniform cppE)
{
  Camera *uniform cam = uniform new Camera;
  cam->cppEquivalent = cppE;
  cam->initRay = PanoramicCamera_initRay;
  cam->doesDOF = false;
  return cam;
}
