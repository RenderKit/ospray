// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.h"
// ispc exports
#include "camera/OrthographicCamera_ispc.h"

namespace ospray {

OrthographicCamera::OrthographicCamera()
{
  getSh()->super.initRay = ispc::OrthographicCamera_initRay_addr();
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
  ispc::OrthographicCamera_projectBox(
      getSh(), (const ispc::box3f &)b, (ispc::box3f &)projection);
  return projection;
}

} // namespace ospray
