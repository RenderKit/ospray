// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDeviceObject.h"
#include "common/FeatureFlagsEnum.h"
#include "common/MotionTransform.h"
#include "common/ObjectFactory.h"
#include "common/StructShared.h"
// ispc shared
#include "CameraShared.h"

namespace ospray {

// base camera class abstraction
// the base class itself does not do anything useful; look into
// perspectivecamera etc for that
struct OSPRAY_SDK_INTERFACE Camera
    : public AddStructShared<ISPCDeviceObject, ispc::Camera>,
      public ObjectFactory<Camera, api::ISPCDevice &>
{
  Camera(api::ISPCDevice &device, const FeatureFlagsOther featureFlags);
  ~Camera() override;

  std::string toString() const override;
  void commit() override;

  // Project the bounding box to the screen
  // The projected box will be returned in normalized [0, 1] coordinates in
  // the framebuffer, the z coordinate will store the min and max depth, in
  // box.lower, box.upper respectively
  // Assume no motion blur nor depth of field (true for SciVis)
  virtual box3f projectBox(const box3f &b) const;

  FeatureFlags getFeatureFlags() const;

  // Data members //

  // if motionBlur in local camera space; otherwise in world-space:
  vec3f pos{0.f, 0.f, 0.f}; // position of the camera
  vec3f dir{0.f, 0.f, 1.f}; // main direction of the camera
  vec3f up{0.f, 1.f, 0.f}; // up direction of the camera

  float nearClip{1e-6f}; // near clipping distance
  // definition of the image region, may even be outside of [0..1]^2
  // to simulate sensor shift
  vec2f imageStart{0.f, 0.f}; // lower left corner
  vec2f imageEnd{1.f, 1.f}; // upper right corner
  range1f shutter{0.5f, 0.5f}; // start and end time of camera shutter time
  float rollingShutterDuration{0.0f};
  OSPShutterType shutterType{OSP_SHUTTER_GLOBAL};

 private:
  RTCGeometry embreeGeometry{nullptr};
  MotionTransform motionTransform;
  FeatureFlagsOther featureFlags{FFO_NONE};
};

OSPTYPEFOR_SPECIALIZATION(Camera *, OSP_CAMERA);

inline FeatureFlags Camera::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.other = featureFlags;
  return ff;
}

} // namespace ospray
