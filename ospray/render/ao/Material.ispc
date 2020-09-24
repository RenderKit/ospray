// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"

export void *uniform AOMaterial_create(void *uniform cppE)
{
  AOMaterial *uniform mat = uniform new uniform AOMaterial;

  mat->super.cppEquivalent = cppE;

  return mat;
}

export void AOMaterial_set(void *uniform _mat,
    uniform float d,
    const uniform TextureParam &dMap,
    const uniform vec3f &Kd,
    const uniform TextureParam &KdMap)
{
  AOMaterial *uniform mat = (AOMaterial * uniform) _mat;

  mat->super.type = RENDERER_TYPE_AO;

  mat->d = d;
  mat->dMap = dMap;
  mat->Kd = Kd;
  mat->KdMap = KdMap;
}
