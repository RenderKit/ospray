// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"

namespace ospray {

/*! \defgroup panoramic_camera The Panoramic Camera ("panoramic")

  \brief Implements a panoramic camera with latitude/longitude mapping,
  without support for Depth of Field or Motion Blur

  \ingroup ospray_supported_cameras

  A simple panoramic camera. This camera type is loaded by passing
  the type string "panoramic" to \ref ospNewCamera

  The panoramic camera supports the following parameters
  <pre>
  vec3f(a) pos;    // camera position
  vec3f(a) dir;    // camera direction
  vec3f(a) up;     // up vector
  </pre>

  The functionality for a panoramic camera is implemented via the
  \ref ospray::PanoramicCamera class.
*/

//! Implements a simple panoramic camera (see \subpage panoramic_camera)
struct OSPRAY_SDK_INTERFACE PanoramicCamera : public Camera
{
  /*! \brief constructor \internal also creates the ispc-side data structure */
  PanoramicCamera();
  virtual ~PanoramicCamera() override = default;

  virtual std::string toString() const override;
};

} // namespace ospray
