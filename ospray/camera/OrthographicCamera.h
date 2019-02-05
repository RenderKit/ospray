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

#include "camera/Camera.h"

namespace ospray {

  /*! \defgroup orthographic_camera The Orthographic Camera ("orthographic")

    \brief Implements a straightforward orthographic camera for orthographic
    projections, without support for Depth of Field or Motion Blur

    \ingroup ospray_supported_cameras

    A simple orthographic camera. This camera type is loaded by passing
    the type string "orthographic" to \ref ospNewCamera

    The orthographic camera supports the following parameters
    <pre>
    vec3f(a) pos;    // camera position
    vec3f(a) dir;    // camera direction
    vec3f(a) up;     // up vector
    float    height; // size of the camera's image plane in y, in world coordinates
    float    aspect; // aspect ratio (x/y)
    </pre>

    The functionality for a orthographic camera is implemented via the
    \ref ospray::OrthographicCamera class.
  */

  //! Implements a simple orthographic camera (see \subpage orthographic_camera)
  struct OSPRAY_SDK_INTERFACE OrthographicCamera : public Camera
  {
    /*! \brief constructor \internal also creates the ispc-side data structure */
    OrthographicCamera();
    ~OrthographicCamera() override = default;

    virtual std::string toString() const override;
    virtual void commit() override;

    // Data members //

    float  height; // size of the camera's image plane in y, in world coordinates
    float  aspect;
  };

} // ::ospray
