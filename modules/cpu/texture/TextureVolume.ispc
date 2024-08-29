// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "volume/Volume.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
// c++ shared
#include "TextureVolumeShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f TextureVolume_get(
    const TextureVolume *uniform self, const vec3f &P)
{
  const Volume *uniform volume = self->volume;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, P)) {
    const float sample = Volume_getSample(volume, P);
    const TransferFunction *uniform tfn = self->transferFunction;
    return TransferFunction_dispatch_get(tfn, sample);
  } else
    return make_vec4f(0.f);
}

///////////////////////////////////////////////////////////////////////////////

export void *uniform TextureVolume_get_addr()
{
  return (void *uniform)TextureVolume_get;
}

OSPRAY_END_ISPC_NAMESPACE

#endif
