// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PerspectiveCamera.h"
// ispc exports
#include "camera/PerspectiveCamera_ispc.h"

namespace ospray {

PerspectiveCamera::PerspectiveCamera()
{
  getSh()->super.initRay = ispc::PerspectiveCamera_initRay_addr();
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
      getSh()->super.initRay = ispc::PerspectiveCamera_initRayMB_addr();
      getSh()->du_size = vec3f(imgPlaneSize.x, imgPlaneSize.y, architectural);
      getSh()->dv_up = up;
      getSh()->ipd_offset =
          vec3f(0.5f * interpupillaryDistance, focusDistance, 0.0f);
    } else {
      getSh()->super.initRay = ispc::PerspectiveCamera_initRay_addr();
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
  ispc::PerspectiveCamera_projectBox(
      getSh(), (const ispc::box3f &)b, (ispc::box3f &)projection);
  return projection;
}

} // namespace ospray
