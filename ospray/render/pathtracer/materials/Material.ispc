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
}

export uniform bool PathTraceMaterial_isEmissive(void *uniform _mat)
{
  PathTraceMaterial *uniform mat = (PathTraceMaterial * uniform) _mat;

  if (mat->material.type != RENDERER_TYPE_PATHTRACER)
    return false;

  return reduce_max(mat->emission) > 0.f;
}
