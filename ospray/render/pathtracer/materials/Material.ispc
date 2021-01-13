// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"

vec3f PathTraceMaterial_getTransparency(
    const uniform PathTraceMaterial *uniform self,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  return make_vec3f(0.0f);
}

void PathTraceMaterial_selectNextMedium(
    const uniform PathTraceMaterial *uniform self,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{ /* do nothing by default */
}

void PathTraceMaterial_Constructor(uniform PathTraceMaterial *uniform self,
    uniform PathTraceMaterial_GetBSDFFunc getBSDF,
    uniform PathTraceMaterial_GetTransparencyFunc getTransparency,
    uniform PathTraceMaterial_SelectNextMediumFunc selectNextMedium)
{
  self->material.type = RENDERER_TYPE_PATHTRACER;

  self->getBSDF = getBSDF;
  self->getTransparency =
      getTransparency ? getTransparency : PathTraceMaterial_getTransparency;
  self->selectNextMedium =
      selectNextMedium ? selectNextMedium : PathTraceMaterial_selectNextMedium;
  self->emission = make_vec3f(0.f);

  uniform TextureParam nulltex = make_TextureParam();
  PathTraceMaterial_SetEnsightTex1D(self, &nulltex, &nulltex);
  PathTraceMaterial_SetEnsightTex2D(self, &nulltex, 0, 0);
}

void PathTraceMaterial_SetEnsightTex1D(
    uniform PathTraceMaterial *uniform self,
    uniform TextureParam *uniform colorbyMap,
    uniform TextureParam *uniform alphabyMap)
{
    if (colorbyMap) {
        self->colorbyMap = *colorbyMap;
    }
    else {
        self->colorbyMap = make_TextureParam();
    }
    if (alphabyMap) {
        self->alphabyMap = *alphabyMap;
    }
    else {
        self->alphabyMap = make_TextureParam();
    }
}

void PathTraceMaterial_SetEnsightTex2D(
    uniform PathTraceMaterial *uniform self,
    uniform TextureParam *uniform tex2dMap,
    uniform int tex2dImageWrap,
    uniform int tex2dImageMode)
{
    if (tex2dMap) {
        self->tex2dMap = *tex2dMap;
    }
    else {
        self->tex2dMap = make_TextureParam();
    }
    self->tex2dImageWrap = tex2dImageWrap,
    self->tex2dImageMode = tex2dImageMode;
}

export uniform bool PathTraceMaterial_isEmissive(void *uniform _mat)
{
  PathTraceMaterial *uniform mat = (PathTraceMaterial * uniform) _mat;

  if (mat->material.type != RENDERER_TYPE_PATHTRACER)
    return false;

  return reduce_max(mat->emission) > 0.f;
}
