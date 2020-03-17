// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.h"
#include "OrthographicCamera_ispc.h"

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

  // ------------------------------------------------------------------
  // first, "parse" the additional expected parameters
  // ------------------------------------------------------------------
  height = getParam<float>("height", 1.f); // imgPlane_size_y
  aspect = getParam<float>("aspect", 1.f);

  // ------------------------------------------------------------------
  // now, update the local precomputed values
  // ------------------------------------------------------------------
  dir = normalize(dir);
  vec3f pos_du = normalize(cross(dir, up));
  vec3f pos_dv = cross(pos_du, dir);

  pos_du *= height * aspect; // imgPlane_size_x
  pos_dv *= height;

  vec3f pos_00 = pos - 0.5f * pos_du - 0.5f * pos_dv;

  ispc::OrthographicCamera_set(getIE(),
      (const ispc::vec3f &)dir,
      (const ispc::vec3f &)pos_00,
      (const ispc::vec3f &)pos_du,
      (const ispc::vec3f &)pos_dv);
}

} // namespace ospray
