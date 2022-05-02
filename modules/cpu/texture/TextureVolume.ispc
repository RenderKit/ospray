// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture.ih"
#include "volume/Volume.ih"
#include "volume/transferFunction/TransferFunctionShared.h"
// c++ shared
#include "TextureVolumeShared.h"

vec4f TextureVolume_get(
    const TextureVolume *uniform self, const varying DifferentialGeometry &dg)
{
  const Volume *uniform volume = self->volume;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, dg.lP)) {
    const float sample = Volume_getSample(volume, dg.lP);
    const TransferFunction *uniform tfn = self->transferFunction;
    return tfn->get(tfn, sample);
  } else
    return make_vec4f(0.f);
}

vec3f TextureVolume_getN(
    const TextureVolume *uniform self, const varying DifferentialGeometry &dg)
{
  // TODO
  return make_vec3f(0.f, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////

export void *uniform TextureVolume_get_addr()
{
  return TextureVolume_get;
}

export void *uniform TextureVolume_getN_addr()
{
  return TextureVolume_getN;
}
