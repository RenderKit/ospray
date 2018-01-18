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

#include "PerspectiveCamera.h"
#include "PerspectiveCamera_ispc.h"

namespace ospray {

  PerspectiveCamera::PerspectiveCamera()
  {
    ispcEquivalent = ispc::PerspectiveCamera_create(this);
  }

  std::string PerspectiveCamera::toString() const
  {
    return "ospray::PerspectiveCamera";
  }

  void PerspectiveCamera::commit()
  {
    Camera::commit();

    // ------------------------------------------------------------------
    // first, "parse" the additional expected parameters
    // ------------------------------------------------------------------
    fovy = getParamf("fovy", 60.f);
    aspect = getParamf("aspect", 1.f);
    apertureRadius = getParamf("apertureRadius", 0.f);
    focusDistance = getParamf("focusDistance", 1.f);
    architectural = getParam1i("architectural", false);
    stereoMode = (StereoMode)getParam1i("stereoMode", OSP_STEREO_NONE);
    // the default 63.5mm represents the average human IPD
    interpupillaryDistance = getParamf("interpupillaryDistance", 0.0635f);

    // ------------------------------------------------------------------
    // now, update the local precomputed values
    // ------------------------------------------------------------------
    dir = normalize(dir);
    vec3f dir_du = normalize(cross(dir, up)); // right-handed coordinate system
    vec3f dir_dv;
    if (architectural)
      dir_dv = normalize(up); // orient film to be parallel to 'up' and shift such that 'dir' is centered
    else
      dir_dv = cross(dir_du, dir); // rotate film to be perpendicular to 'dir'

    vec3f org = pos;
    const vec3f ipd_offset = 0.5f * interpupillaryDistance * dir_du;

    switch (stereoMode) {
      case OSP_STEREO_LEFT:
        org -= ipd_offset;
        break;
      case OSP_STEREO_RIGHT:
        org += ipd_offset;
        break;
      case OSP_STEREO_SIDE_BY_SIDE:
        aspect *= 0.5f;
        break;
      case OSP_STEREO_NONE:
        break;
    }

    float imgPlane_size_y = 2.f*tanf(deg2rad(0.5f*fovy));
    float imgPlane_size_x = imgPlane_size_y * aspect;

    dir_du *= imgPlane_size_x;
    dir_dv *= imgPlane_size_y;

    vec3f dir_00 = dir - .5f * dir_du - .5f * dir_dv;

    float scaledAperture = 0.f;
    // prescale to focal plane
    if (apertureRadius > 0.f) {
      dir_du *= focusDistance;
      dir_dv *= focusDistance;
      dir_00 *= focusDistance;
      scaledAperture = apertureRadius / (imgPlane_size_x * focusDistance);
    }

    ispc::PerspectiveCamera_set(getIE()
        , (const ispc::vec3f&)org
        , (const ispc::vec3f&)dir_00
        , (const ispc::vec3f&)dir_du
        , (const ispc::vec3f&)dir_dv
        , scaledAperture
        , aspect
        , stereoMode == OSP_STEREO_SIDE_BY_SIDE
        , (const ispc::vec3f&)ipd_offset
        );
  }

  OSP_REGISTER_CAMERA(PerspectiveCamera,perspective);
  OSP_REGISTER_CAMERA(PerspectiveCamera,thinlens);
  OSP_REGISTER_CAMERA(PerspectiveCamera,stereo);

} // ::ospray
