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

// ospray
#include "Camera.h"
#include "common/Util.h"
#include "Camera_ispc.h"

namespace ospray {

  ProjectedPoint::ProjectedPoint(const vec3f &pos, float radius)
    : screenPos(pos), radius(radius)
  {}

  Camera *Camera::createInstance(const char *type)
  {
    return createInstanceHelper<Camera, OSP_CAMERA>(type);
  }

  std::string Camera::toString() const
  {
    return "ospray::Camera";
  }

  void Camera::commit()
  {
    // "parse" the general expected parameters
    pos      = getParam3f("pos", vec3f(0.f));
    dir      = getParam3f("dir", vec3f(0.f, 0.f, 1.f));
    up       = getParam3f("up", vec3f(0.f, 1.f, 0.f));
    nearClip = getParam1f("nearClip", getParam1f("near_clip", 1e-6f));

    imageStart = getParam2f("imageStart", getParam2f("image_start", vec2f(0.f)));
    imageEnd   = getParam2f("imageEnd", getParam2f("image_end", vec2f(1.f)));

    shutterOpen = getParam1f("shutterOpen", 0.0f);
    shutterClose = getParam1f("shutterClose", 0.0f);

    linear3f frame;
    frame.vz = -normalize(dir);
    frame.vx = normalize(cross(up, frame.vz));
    frame.vy = cross(frame.vz, frame.vx);

    ispc::Camera_set(getIE()
        , (const ispc::vec3f&)pos
        , (const ispc::LinearSpace3f&)frame
        , nearClip
        , (const ispc::vec2f&)imageStart
        , (const ispc::vec2f&)imageEnd
        , shutterOpen
        , shutterClose
        );
  }

  ProjectedPoint Camera::projectPoint(const vec3f &) const {
    NOTIMPLEMENTED;
  }

} // ::ospray

