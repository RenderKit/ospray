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
    void *uniform _map_Kd,
    const uniform affine2f &xform_Kd,
    const uniform vec3f &Kd)
{
  SciVisMaterial *uniform mat = (SciVisMaterial * uniform) _mat;

  mat->d = d;
  mat->map_Kd = make_TextureParam((uniform Texture2D *)_map_Kd, xform_Kd);
  mat->Kd = Kd;
}
