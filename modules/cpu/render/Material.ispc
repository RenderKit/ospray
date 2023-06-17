// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec3f Material_getTransparency(const uniform Material *uniform,
    const DifferentialGeometry &,
    const Ray &,
    const Medium &)
{
  return make_vec3f(0.0f);
}

SYCL_EXTERNAL void Material_selectNextMedium(
    const uniform Material *uniform, const DifferentialGeometry &, Medium &)
{ /* do nothing by default */
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Material_getTransparency_addr()
{
  return (void *uniform)Material_getTransparency;
}

export void *uniform Material_selectNextMedium_addr()
{
  return (void *uniform)Material_selectNextMedium;
}

OSPRAY_END_ISPC_NAMESPACE
