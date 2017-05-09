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

#include "PanoramicCamera.h"
#include "PanoramicCamera_ispc.h"

namespace ospray {

  PanoramicCamera::PanoramicCamera()
  {
    ispcEquivalent = ispc::PanoramicCamera_create(this);
  }

  std::string PanoramicCamera::toString() const
  {
    return "ospray::PanoramicCamera";
  }

  void PanoramicCamera::commit()
  {
    Camera::commit();

    // ------------------------------------------------------------------
    // update the local precomputed values: the camera coordinate frame
    // ------------------------------------------------------------------
    linear3f frame;
    frame.vx = normalize(dir);
    frame.vy = normalize(cross(frame.vx, up));
    frame.vz = cross(frame.vx, frame.vy);

    ispc::PanoramicCamera_set(getIE(),
                              (const ispc::vec3f&)pos,
                              (const ispc::LinearSpace3f&)frame);
  }

  OSP_REGISTER_CAMERA(PanoramicCamera,panoramic);

} // ::ospray
