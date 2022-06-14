// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
// c++ shared
#include "LightShared.h"

Light_EvalRes Light_eval(const Light *uniform,
    const DifferentialGeometry &,
    const vec3f &,
    const float,
    const float,
    const float)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform Light_eval_addr()
{
  return Light_eval;
}
