// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "camera/Camera.h"

namespace ospray {

  /*! \defgroup perspective_camera The Perspective Camera ("perspective")

    \brief Implements a straightforward perspective (or "pinhole"
    camera) for perspective rendering, without support for Depth of Field or Motion Blur

    \ingroup ospray_supported_cameras
    
    A simple perspective camera. This camera type is loaded by passing
    the type string "perspective" to \ref ospNewCamera

    The perspective camera supports the following parameters
    <pre>
    vec3f(a) pos;    // camera position
    vec3f(a) dir;    // camera direction
    vec3f(a) up;     // up vector
    float    fovy;   // field of view (camera opening angle) in frame's y dimension
    float    aspect; // aspect ratio (x/y)
    </pre>

    The functionality for a perspective camera is implemented via the
    \ref ospray::PerspectiveCamera class.
  */

  //! Implements a simple perspective camera (see \subpage perspective_camera)
  struct PerspectiveCamera : public Camera {
    /*! \brief constructor \internal also creates the ispc-side data structure */
    PerspectiveCamera();
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::PerspectiveCamera"; }
    virtual void commit();
    
  public:
    // ------------------------------------------------------------------
    // the parameters we 'parsed' from our parameters
    // ------------------------------------------------------------------
    float fovy;
    float aspect;
    float apertureRadius;
    float focusDistance;
  };

} // ::ospray
