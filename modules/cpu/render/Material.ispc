// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"

vec3f Material_getTransparency(const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  return make_vec3f(0.0f);
}

void Material_selectNextMedium(const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{ /* do nothing by default */
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Material_getTransparency_addr()
{
  return Material_getTransparency;
}

export void *uniform Material_selectNextMedium_addr()
{
  return Material_selectNextMedium;
}
