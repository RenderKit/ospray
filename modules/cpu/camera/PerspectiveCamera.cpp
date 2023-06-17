// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PerspectiveCamera.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "camera/PerspectiveCamera_ispc.h"
#endif

namespace ospray {

PerspectiveCamera::PerspectiveCamera(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_CAMERA_PERSPECTIVE)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.initRay = reinterpret_cast<ispc::Camera_initRay>(
      ispc::PerspectiveCamera_initRay_addr());
#endif
}

std::string PerspectiveCamera::toString() const
{
  return "ospray::PerspectiveCamera";
}

void PerspectiveCamera::commit()
{
  Camera::commit();

  fovy = getParam<float>("fovy", 60.f);
  aspect = getParam<float>("aspect", 1.f);
  apertureRadius = getParam<float>("apertureRadius", 0.f);
  focusDistance = getParam<float>("focusDistance", 1.f);
  architectural = getParam<bool>("architectural", false);
  stereoMode = (OSPStereoMode)getParam<uint8_t>(
      "stereoMode", getParam<int32_t>("stereoMode", OSP_STEREO_NONE));
  // the default 63.5mm represents the average human IPD
  interpupillaryDistance = getParam<float>("interpupillaryDistance", 0.0635f);

  switch (stereoMode) {
  case OSP_STEREO_SIDE_BY_SIDE:
    aspect *= 0.5f;
    break;
  case OSP_STEREO_TOP_BOTTOM:
    aspect *= 2.f;
    break;
  default:
    break;
  }

  vec2f imgPlaneSize;
  imgPlaneSize.y = 2.f * tanf(deg2rad(0.5f * fovy));
  imgPlaneSize.x = imgPlaneSize.y * aspect;

  // Set shared structure members
  {
    getSh()->scaledAperture = apertureRadius / (imgPlaneSize.x * focusDistance);
    getSh()->aspect = aspect;
    getSh()->stereoMode = stereoMode;
    getSh()->dir_00 = normalize(dir);
    getSh()->org = pos;
    getSh()->imgPlaneSize = imgPlaneSize;

    if (getSh()->super.motionBlur) {
#ifndef OSPRAY_TARGET_SYCL
      getSh()->super.initRay = reinterpret_cast<ispc::Camera_initRay>(
          ispc::PerspectiveCamera_initRayMB_addr());
#endif
      getSh()->du_size = vec3f(imgPlaneSize.x, imgPlaneSize.y, architectural);
      getSh()->dv_up = up;
      getSh()->ipd_offset =
          vec3f(0.5f * interpupillaryDistance, focusDistance, 0.0f);
    } else {
#ifndef OSPRAY_TARGET_SYCL
      getSh()->super.initRay = reinterpret_cast<ispc::Camera_initRay>(
          ispc::PerspectiveCamera_initRay_addr());
#endif
      getSh()->du_size = normalize(cross(getSh()->dir_00, up));
      if (architectural) // orient film to be parallel to 'up'
        getSh()->dv_up = normalize(up);
      else // rotate film to be perpendicular to 'dir'
        getSh()->dv_up = cross(getSh()->du_size, getSh()->dir_00);

      getSh()->ipd_offset = 0.5f * interpupillaryDistance * getSh()->du_size;

      switch (stereoMode) {
      case OSP_STEREO_LEFT:
        getSh()->org = getSh()->org - getSh()->ipd_offset;
        break;
      case OSP_STEREO_RIGHT:
        getSh()->org = getSh()->org + getSh()->ipd_offset;
        break;
      case OSP_STEREO_TOP_BOTTOM:
        // flip offset to have left eye at top (image coord origin at lower
        // left)
        getSh()->ipd_offset = -getSh()->ipd_offset;
        break;
      default:
        break;
      }

      getSh()->du_size = getSh()->du_size * imgPlaneSize.x;
      getSh()->dv_up = getSh()->dv_up * imgPlaneSize.y;
      getSh()->dir_00 =
          getSh()->dir_00 - 0.5f * getSh()->du_size - 0.5f * getSh()->dv_up;

      // prescale to focal plane
      if (getSh()->scaledAperture > 0.f) {
        getSh()->du_size = getSh()->du_size * focusDistance;
        getSh()->dv_up = getSh()->dv_up * focusDistance;
        getSh()->dir_00 = getSh()->dir_00 * focusDistance;
      }
    }
  }
}

box3f PerspectiveCamera::projectBox(const box3f &b) const
{
  if (stereoMode != OSP_STEREO_NONE) {
    return box3f(vec3f(0.f), vec3f(1.f));
  }
  box3f projection;
  const vec3f dir = normalize(
      getSh()->dir_00 + 0.5f * getSh()->du_size + 0.5f * getSh()->dv_up);
  const vec3f dun = normalize(getSh()->du_size) / getSh()->imgPlaneSize.x;
  const vec3f dvn = normalize(getSh()->dv_up) / getSh()->imgPlaneSize.y;

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

    // We find the intersection of the ray through the point with the virtual
    // film plane, then find the vector to this point from the origin of the
    // film plane (screenDir) and project this point onto the x/y axes of
    // the plane.
    const vec3f v = p - getSh()->org;
    const vec3f r = normalize(v);
    const float denom = dot(-r, -dir);
    if (denom != 0.f) {
      float t = 1.f / denom;
      const vec3f screenDir = r * t - getSh()->dir_00;
      projectedPt.x = dot(screenDir, dun);
      projectedPt.y = dot(screenDir, dvn);
      projectedPt.z = std::signbit(t) ? -length(v) : length(v);
      projection.lower.x = min(projectedPt.x, projection.lower.x);
      projection.lower.y = min(projectedPt.y, projection.lower.y);
      projection.lower.z = min(projectedPt.z, projection.lower.z);

      projection.upper.x = max(projectedPt.x, projection.upper.x);
      projection.upper.y = max(projectedPt.y, projection.upper.y);
      projection.upper.z = max(projectedPt.z, projection.upper.z);
    }
  }

  // If some points are behind and some are in front mark the box
  // as covering the full screen
  if (projection.lower.z < 0.f && projection.upper.z > 0.f) {
    projection.lower.x = 0.f;
    projection.lower.y = 0.f;

    projection.upper.x = 1.f;
    projection.upper.y = 1.f;
  }
  return projection;
}

} // namespace ospray
