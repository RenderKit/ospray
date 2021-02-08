// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"

Light_EvalRes Light_eval(const uniform Light *uniform,
    const DifferentialGeometry &,
    const vec3f &,
    const float,
    const float)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side Light object
export void Light_set(void *uniform _self,
    const uniform bool isVisible)
{
  Light *uniform self = (Light * uniform) _self;
  self->isVisible = isVisible;
}
