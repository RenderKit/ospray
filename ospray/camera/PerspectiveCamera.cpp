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

#include "PerspectiveCamera.h"
#include <limits>
// ispc-side stuff
#include "PerspectiveCamera_ispc.h"
#include <limits>


#ifdef __WIN32__
#  define _USE_MATH_DEFINES
#  include <math.h> // M_PI
#endif

namespace ospray {

  PerspectiveCamera::PerspectiveCamera()
  {
    ispcEquivalent = ispc::PerspectiveCamera_create(this);
  }
  void PerspectiveCamera::commit()
  {
    // ------------------------------------------------------------------
    // first, "parse" the expected parameters
    // ------------------------------------------------------------------
    pos    = getParam3f("pos",vec3f(0.f));
    dir    = getParam3f("dir",vec3f(0.f,0.0f,1.f));
    up     = getParam3f("up", vec3f(0.f,1.0f,0.f));
    near   = getParamf("near",0.f);
    far    = getParamf("far", std::numeric_limits<float>::infinity());
    fovy   = getParamf("fovy",60.f);
    aspect = getParamf("aspect",1.f);
    nearClip = getParam1f("near_clip",getParam1f("nearClip",1e-6f));


    // ------------------------------------------------------------------
    // now, update the local precomptued values
    // ------------------------------------------------------------------
    vec3f dz = normalize(dir);
    vec3f dx = normalize(cross(dz,up));
    vec3f dy = normalize(cross(dx,dz));

    float imgPlane_size_y = 2.f*tanf(fovy/2.f*M_PI/180.);
    float imgPlane_size_x = imgPlane_size_y * aspect;
    dir_00
      = dz
      - (.5f * imgPlane_size_x) * dx
      - (.5f * imgPlane_size_y) * dy;
    dir_du = dx * imgPlane_size_x;
    dir_dv = dy * imgPlane_size_y;

    ispc::PerspectiveCamera_set(getIE(),
                                (const ispc::vec3f&)pos,
                                (const ispc::vec3f&)dir_00,
                                (const ispc::vec3f&)dir_du,
                                (const ispc::vec3f&)dir_dv,
                                nearClip);
  }

  void PerspectiveCamera::initRay(Ray &ray, const vec2f &sample)
  {
    ray.org = pos;
    ray.dir = dir_00
      + sample.x * dir_du
      + sample.y * dir_dv;

    ray.dir = normalize(ray.dir);

    if (ray.dir.x == 0.f) ray.dir.x = 1e-6f;
    if (ray.dir.y == 0.f) ray.dir.y = 1e-6f;
    if (ray.dir.z == 0.f) ray.dir.z = 1e-6f;


    ray.t0  = 1e-6f;
    ray.t   = std::numeric_limits<float>::infinity();
    ray.time = 0.f;
    ray.mask = -1;
    ray.geomID = -1;
    ray.primID = -1;
    ray.instID = -1;
  }

  OSP_REGISTER_CAMERA(PerspectiveCamera,perspective);

} // ::ospray
