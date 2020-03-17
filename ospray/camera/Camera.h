// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/Util.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE ProjectedPoint
{
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
  Camera();
  virtual ~Camera() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  // Project the world space point to the screen, and return the screen-space
  // point (in normalized screen-space coordinates) along with the depth
  virtual ProjectedPoint projectPoint(const vec3f &p) const;

  static Camera *createInstance(const char *identifier);
  template <typename T>
  static void registerType(const char *type);

  // Data members //

  vec3f pos; // position of the camera in world-space
  vec3f dir; // main direction of the camera in world-space
  vec3f up; // up direction of the camera in world-space
  float nearClip; // near clipping distance
  // definition of the image region, may even be outside of [0..1]^2
  // to simulate sensor shift
  vec2f imageStart; // lower left corner
  vec2f imageEnd; // upper right corner
  float shutterOpen; // start time of camera shutter
  float shutterClose; // end time of camera shutter

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<Camera> f);
};

OSPTYPEFOR_SPECIALIZATION(Camera *, OSP_CAMERA);

// Inlined defintions /////////////////////////////////////////////////////////

template <typename T>
inline void Camera::registerType(const char *type)
{
  registerTypeHelper<Camera, T>(type);
}

} // namespace ospray
