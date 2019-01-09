// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#pragma once

#include "common/Managed.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE ProjectedPoint {
    //! The screen space position and depth
    vec3f screenPos;
    //! The radius of the projected disk, in the case of depth of field
    float radius;

    ProjectedPoint(const vec3f &pos, float radius);
  };

  //! base camera class abstraction
  /*! the base class itself does not do anything useful; look into
      perspectivecamera etc for that */
  struct OSPRAY_SDK_INTERFACE Camera : public ManagedObject
  {
    virtual ~Camera() override = default;

    virtual std::string toString() const override;

    virtual void commit() override;

    // Project the world space point to the screen, and return the screen-space
    // point (in normalized screen-space coordinates) along with the depth
    virtual ProjectedPoint projectPoint(const vec3f &p) const;

    static Camera *createInstance(const char *identifier);

    // Data members //

    vec3f  pos;      // position of the camera in world-space
    vec3f  dir;      // main direction of the camera in world-space
    vec3f  up;       // up direction of the camera in world-space
    float  nearClip; // near clipping distance
    // definition of the image region, may even be outside of [0..1]^2
    // to simulate sensor shift
    vec2f  imageStart; // lower left corner
    vec2f  imageEnd; // upper right corner
    float shutterOpen; // start time of camera shutter
    float shutterClose; // end time of camera shutter
  };

  /*! \brief registers a internal ospray::'ClassName' camera under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      camera. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this camera.
  */
#define OSP_REGISTER_CAMERA(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(::ospray::Camera, camera, InternalClass, external_name)

} // ::ospray
