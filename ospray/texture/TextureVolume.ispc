// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.ih"

#include "volume/VolumetricModel.ih"

vec4f TextureVolume_get(
    const Texture2D *uniform self, const varying DifferentialGeometry &dg)
{
  VolumetricModel *uniform volumetricModel =
      (VolumetricModel * uniform) self->data;
  const uniform box3f bounds = volumetricModel->boundingBox;

  if (box_contains(bounds, dg.P)) {
    Volume *uniform volume = (Volume * uniform) volumetricModel->volume;
    const float sample = Volume_getSample(volume, dg.P);
    TransferFunction *uniform tfn = volumetricModel->transferFunction;
    return tfn->get(tfn, sample);
  } else
    return make_vec4f(0.f);
}

vec3f TextureVolume_getN(
    const Texture2D *uniform self, const varying DifferentialGeometry &dg)
{
  // TODO
  return make_vec3f(0.f, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////

export void *uniform TextureVolume_create(void *uniform volume)
{
  Texture2D *uniform self = uniform new uniform Texture2D;
  self->data = volume;
  self->get = TextureVolume_get;
  self->getNormal = TextureVolume_getN;

  return self;
}
