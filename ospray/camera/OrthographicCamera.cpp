// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "OrthographicCamera.h"
#include "OrthographicCamera_ispc.h"

namespace ospray {

  OrthographicCamera::OrthographicCamera()
  {
    ispcEquivalent = ispc::OrthographicCamera_create(this);
  }

  std::string OrthographicCamera::toString() const
  {
    return "ospray::OrthographicCamera";
  }

  void OrthographicCamera::commit()
  {
    Camera::commit();

    // ------------------------------------------------------------------
    // first, "parse" the additional expected parameters
    // ------------------------------------------------------------------
    height = getParamf("height", 1.f); // imgPlane_size_y
    aspect = getParamf("aspect", 1.f);

    // ------------------------------------------------------------------
    // now, update the local precomputed values
    // ------------------------------------------------------------------
    dir = normalize(dir);
    vec3f pos_du = normalize(cross(dir, up));
    vec3f pos_dv = cross(pos_du, dir);

    pos_du *= height * aspect; // imgPlane_size_x
    pos_dv *= height;

    vec3f pos_00 = pos - 0.5f * pos_du - 0.5f * pos_dv; 

    ispc::OrthographicCamera_set(getIE(),
                                 (const ispc::vec3f&)dir,
                                 (const ispc::vec3f&)pos_00,
                                 (const ispc::vec3f&)pos_du,
                                 (const ispc::vec3f&)pos_dv);
  }

  OSP_REGISTER_CAMERA(OrthographicCamera, orthographic);

} // ::ospray
