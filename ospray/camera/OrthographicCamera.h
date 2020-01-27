// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
  float    height; // size of the camera's image plane in y, in world
  coordinates float    aspect; // aspect ratio (x/y)
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

  float height; // size of the camera's image plane in y, in world coordinates
  float aspect;
};

} // namespace ospray
