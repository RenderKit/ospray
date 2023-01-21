// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "camera/OrthographicCamera_ispc.h"
#endif

namespace ospray {

OrthographicCamera::OrthographicCamera(api::ISPCDevice &device)
    : AddStructShared(
        device.getIspcrtContext(), device, FFO_CAMERA_ORTHOGRAPHIC)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.initRay = reinterpret_cast<ispc::Camera_initRay>(
      ispc::OrthographicCamera_initRay_addr());
#endif
}

std::string OrthographicCamera::toString() const
{
  return "ospray::OrthographicCamera";
}

void OrthographicCamera::commit()
{
  Camera::commit();

  height = getParam<float>("height", 1.f);
  aspect = getParam<float>("aspect", 1.f);

  vec2f size = height;
  size.x *= aspect;

  if (getSh()->super.motionBlur) {
    getSh()->dir = dir;
    getSh()->du_size = vec3f(size.x, size.y, 1.0f);
    getSh()->dv_up = up;
    getSh()->org = pos;
  } else {
    getSh()->dir = normalize(dir);
    getSh()->du_size = normalize(cross(getSh()->dir, up));
    getSh()->dv_up = cross(getSh()->du_size, getSh()->dir) * size.y;
    getSh()->du_size = getSh()->du_size * size.x;
    getSh()->org =
        pos - 0.5f * getSh()->du_size - 0.5f * getSh()->dv_up; // shift
  }
}

box3f OrthographicCamera::projectBox(const box3f &b) const
{
  box3f projection;
  // normalize to image plane size
  const vec3f dun = getSh()->du_size / dot(getSh()->du_size, getSh()->du_size);
  const vec3f dvn = getSh()->dv_up / dot(getSh()->dv_up, getSh()->dv_up);

  vec3f projectedPt(-1.f, -1.f, 1e20f);
  for (uint32_t i = 0; i < 8; ++i) {
    // Get the point we should be projecting
    vec3f p;
    switch (i) {
    case 0:
      p = b.lower;
      break;
    case 1:
      p.x = b.upper.x;
      p.y = b.lower.y;
      p.z = b.lower.z;
      break;
    case 2:
      p.x = b.upper.x;
      p.y = b.upper.y;
      p.z = b.lower.z;
      break;
    case 3:
      p.x = b.lower.x;
      p.y = b.upper.y;
      p.z = b.lower.z;
      break;
    case 4:
      p.x = b.lower.x;
      p.y = b.lower.y;
      p.z = b.upper.z;
      break;
    case 5:
      p.x = b.upper.x;
      p.y = b.lower.y;
      p.z = b.upper.z;
      break;
    case 6:
      p = b.upper;
      break;
    case 7:
      p.x = b.lower.x;
      p.y = b.upper.y;
      p.z = b.upper.z;
      break;
    }

    // Project the point on to the film plane
    const float depth = dot(p - getSh()->org, getSh()->dir);
    const vec3f screenPt = p - depth * getSh()->dir;
    const vec3f screenDir = screenPt - getSh()->org;
    projectedPt.x = dot(screenDir, dun);
    projectedPt.y = dot(screenDir, dvn);
    projectedPt.z = depth;
    projection.lower.x = min(projectedPt.x, projection.lower.x);
    projection.lower.y = min(projectedPt.y, projection.lower.y);
    projection.lower.z = min(projectedPt.z, projection.lower.z);

    projection.upper.x = max(projectedPt.x, projection.upper.x);
    projection.upper.y = max(projectedPt.y, projection.upper.y);
    projection.upper.z = max(projectedPt.z, projection.upper.z);
  }

  return projection;
}

} // namespace ospray
