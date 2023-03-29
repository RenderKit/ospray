// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "Texture.ih"
#include "volume/Volume.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
// c++ shared
#include "TextureVolumeShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f TextureVolume_get(
    const Texture *uniform _self, const varying DifferentialGeometry &dg)
{
  const TextureVolume *uniform self = (const TextureVolume *uniform)_self;
  const Volume *uniform volume = self->volume;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, dg.lP)) {
    const float sample = Volume_getSample(volume, dg.lP);
    const TransferFunction *uniform tfn = self->transferFunction;
    return TransferFunction_dispatch_get(tfn, sample);
  } else
    return make_vec4f(0.f);
}

SYCL_EXTERNAL vec3f TextureVolume_getNormal(
    const Texture *uniform, const varying DifferentialGeometry &)
{
  // TODO
  return make_vec3f(0.f, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////

export void *uniform TextureVolume_get_addr()
{
  return (void *uniform)TextureVolume_get;
}

export void *uniform TextureVolume_getN_addr()
{
  return (void *uniform)TextureVolume_getNormal;
}

OSPRAY_END_ISPC_NAMESPACE

#endif
