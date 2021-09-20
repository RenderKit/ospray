// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.h"
#include "camera/OrthographicCamera_ispc.h"

namespace ospray {

OrthographicCamera::OrthographicCamera()
{
  ispcEquivalent = ispc::OrthographicCamera_create(this);
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

  ispc::OrthographicCamera_set(getIE(),
      (const ispc::vec3f &)pos,
      (const ispc::vec3f &)dir,
      (const ispc::vec3f &)up,
      (const ispc::vec2f &)size);
}

box3f OrthographicCamera::projectBox(const box3f &b) const
{
  if (motionBlur) {
    return box3f(vec3f(0.f), vec3f(1.f));
  }
  box3f projection;
  ispc::OrthographicCamera_projectBox(
      getIE(), (const ispc::box3f &)b, (ispc::box3f &)projection);
  return projection;
}

} // namespace ospray
