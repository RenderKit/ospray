// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisMaterial.h"
// ispc
#include "SciVisMaterial_ispc.h"

namespace ospray {

SciVisMaterial::SciVisMaterial()
{
  ispcEquivalent = ispc::SciVisMaterial_create(this);
}

void SciVisMaterial::commit()
{
  Kd = getParam<vec3f>("kd", vec3f(.8f));
  d = getParam<float>("d", 1.f);
  map_Kd = (Texture2D *)getParamObject("map_kd");
  ispc::SciVisMaterial_set(
      getIE(), (const ispc::vec3f &)Kd, d, map_Kd ? map_Kd->getIE() : nullptr);
}

} // namespace ospray
