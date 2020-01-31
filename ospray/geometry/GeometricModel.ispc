// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "GeometricModel.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

export void *uniform GeometricModel_create(void *uniform cppE)
{
  GeometricModel *uniform self = uniform new GeometricModel;

  self->geom = NULL;

  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->index);
  Data1D_Constructor(&self->material);
  self->useRendererMaterialList = false;

  self->areaPDF = 0.f;

  return self;
}

export void GeometricModel_set(void *uniform _self,
    void *uniform _geom,
    const Data1D *uniform color,
    const Data1D *uniform index,
    const Data1D *uniform material,
    const uniform bool useRendererMaterialList)
{
  GeometricModel *uniform self = (GeometricModel * uniform) _self;

  self->geom = (Geometry * uniform) _geom;

  self->color = *color;
  self->index = *index;
  self->material = *material;

  self->useRendererMaterialList = useRendererMaterialList;
}
