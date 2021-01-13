// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.ih"

#include "volume/VolumetricModel.ih"

struct TextureVolumeData
{
  const Volume *volume; //!< instanced Volume

  //! Color and opacity transfer function.
  const TransferFunction *transferFunction;
};

vec4f TextureVolume_get(
    const Texture2D *uniform self, const varying DifferentialGeometry &dg)
{
  const TextureVolumeData *uniform data =
      (TextureVolumeData * uniform) self->data;
  const Volume *uniform volume = data->volume;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, dg.lP)) {
    const float sample = Volume_getSample(volume, dg.lP);
    const TransferFunction *uniform tfn = data->transferFunction;
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

export void *uniform TextureVolume_create_deprecated(
    void *uniform _volumetricModel)
{
  Texture2D *uniform self = uniform new uniform Texture2D;
  const VolumetricModel *uniform volumetricModel =
      (VolumetricModel * uniform) _volumetricModel;
  TextureVolumeData *uniform data = uniform new uniform TextureVolumeData;
  data->volume = volumetricModel->volume;
  data->transferFunction = volumetricModel->transferFunction;
  self->data = data;
  self->get = TextureVolume_get;
  self->getNormal = TextureVolume_getN;
  return self;
}

export void *uniform TextureVolume_create(
    void *uniform volume, void *uniform transferFunction)
{
  Texture2D *uniform self = uniform new uniform Texture2D;
  TextureVolumeData *uniform data = uniform new uniform TextureVolumeData;
  data->volume = (const Volume *uniform)volume;
  data->transferFunction = (const TransferFunction *uniform)transferFunction;
  self->data = data;
  self->get = TextureVolume_get;
  self->getNormal = TextureVolume_getN;
  return self;
}

export void TextureVolume_delete(void *uniform _self)
{
  if (_self) {
    Texture2D *uniform self = (Texture2D * uniform) _self;
    delete self->data;
  }
  delete _self;
}
