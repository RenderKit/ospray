// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisMaterial.ih"

export void *uniform SciVisMaterial_create(void *uniform cppE)
{
  SciVisMaterial *uniform mat = uniform new uniform SciVisMaterial;

  mat->super.cppEquivalent = cppE;

  return mat;
}

export void SciVisMaterial_set(void *uniform _mat,
    uniform float d,
    const uniform TextureParam &dMap,
    const uniform vec3f &Kd,
    const uniform TextureParam &KdMap)
{
  SciVisMaterial *uniform mat = (SciVisMaterial * uniform) _mat;

  mat->d = d;
  mat->dMap = dMap;
  mat->Kd = Kd;
  mat->KdMap = KdMap;
}
