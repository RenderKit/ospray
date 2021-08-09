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

} // namespace ospray
