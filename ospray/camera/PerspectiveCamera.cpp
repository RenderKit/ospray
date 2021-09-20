// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PerspectiveCamera.h"
#include "camera/PerspectiveCamera_ispc.h"

namespace ospray {

PerspectiveCamera::PerspectiveCamera()
{
  ispcEquivalent = ispc::PerspectiveCamera_create(this);
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

  ispc::PerspectiveCamera_set(getIE(),
      (const ispc::vec3f &)pos,
      (const ispc::vec3f &)dir,
      (const ispc::vec3f &)up,
      (const ispc::vec2f &)imgPlaneSize,
      apertureRadius / (imgPlaneSize.x * focusDistance),
      focusDistance,
      aspect,
      architectural,
      stereoMode,
      interpupillaryDistance);
}

box3f PerspectiveCamera::projectBox(const box3f &b) const
{
  if (motionBlur || stereoMode != OSP_STEREO_NONE || focusDistance != 1.f) {
    return box3f(vec3f(0.f), vec3f(1.f));
  }
  box3f projection;
  ispc::PerspectiveCamera_projectBox(
      getIE(), (const ispc::box3f &)b, (ispc::box3f &)projection);
  return projection;
}

} // namespace ospray
