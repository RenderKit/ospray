// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisMaterial.ih"

export void *uniform SciVisMaterial_create(void *uniform cppE)
{
  SciVisMaterial *uniform mat = uniform new uniform SciVisMaterial;

  mat->super.cppEquivalent = cppE;

  return mat;
}

export void SciVisMaterial_set(void *uniform _mat,
    const uniform vec3f &Kd,
    uniform float d,
    void *uniform _map_Kd)
{
  SciVisMaterial *uniform mat = (SciVisMaterial * uniform) _mat;

  mat->Kd = Kd;
  mat->map_Kd = (uniform Texture2D * uniform) _map_Kd;
  mat->d = d;
}
