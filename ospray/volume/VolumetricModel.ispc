// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "VolumetricModel.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

export void *uniform VolumetricModel_create(
    void *uniform cppE, void *uniform volume)
{
  VolumetricModel *uniform self = uniform new VolumetricModel;

  self->volume = (Volume * uniform) volume;

  return self;
}

export void VolumetricModel_set(void *uniform _self,
    void *uniform transferFunction,
    const uniform box3f &bounds,
    uniform float densityScale,
    uniform float anisotropy)
{
  VolumetricModel *uniform self = (VolumetricModel * uniform) _self;

  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->boundingBox = bounds;
  self->densityScale = densityScale;
  self->anisotropy = anisotropy;
}

export void VolumetricModel_set_valueSelector(
    void *uniform _self, void *uniform valueSelector)
{
  VolumetricModel *uniform self = (VolumetricModel * uniform) _self;
  self->vklValueSelector = (VKLValueSelector)valueSelector;
}
