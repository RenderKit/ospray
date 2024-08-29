// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureDispatch.ih"
#include "TextureParam.ih"
#include "common/DifferentialGeometry.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline vec4f get4f(const uniform TextureParam uniform &tp,
    const varying DifferentialGeometry &dg)
{
  DifferentialGeometry xfmDg = dg;
  if (tp.transformFlags & TRANSFORM_FLAG_2D) {
    xfmDg.st = xfmPoint(tp.xform2f, dg.st);
    xfmDg.pixelFootprint *= tp.scale2f;
  }
  if (tp.transformFlags & TRANSFORM_FLAG_3D)
    xfmDg.lP = xfmPoint(tp.xform3f, dg.lP);

  return Texture_dispatch_get(tp.ptr, xfmDg);
}

OSPRAY_END_ISPC_NAMESPACE
