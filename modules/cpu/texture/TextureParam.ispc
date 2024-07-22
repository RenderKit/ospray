// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureParam.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

#define TEXTURE_TRANSFORM(fn)                                                  \
  DifferentialGeometry xfmDg = dg;                                             \
  if (tp.transformFlags & TRANSFORM_FLAG_2D) {                                 \
    xfmDg.st = xfmPoint(tp.xform2f, dg.st);                                    \
    xfmDg.pixelFootprint *= tp.scale2f;                                        \
  }                                                                            \
  if (tp.transformFlags & TRANSFORM_FLAG_3D)                                   \
    xfmDg.lP = xfmPoint(tp.xform3f, dg.lP);                                    \
  return fn(tp.ptr, xfmDg)

SYCL_EXTERNAL __noinline float get1f(const uniform TextureParam uniform &tp,
    const varying DifferentialGeometry &dg)
{
  TEXTURE_TRANSFORM(get1f);
}

SYCL_EXTERNAL __noinline vec3f get3f(const uniform TextureParam uniform &tp,
    const varying DifferentialGeometry &dg)
{
  TEXTURE_TRANSFORM(get3f);
}

SYCL_EXTERNAL __noinline vec4f get4f(const uniform TextureParam uniform &tp,
    const varying DifferentialGeometry &dg)
{
  TEXTURE_TRANSFORM(get4f);
}

SYCL_EXTERNAL __noinline vec3f getNormal(const uniform TextureParam uniform &tp,
    const varying DifferentialGeometry &dg)
{
  TEXTURE_TRANSFORM(getNormal);
}
OSPRAY_END_ISPC_NAMESPACE
