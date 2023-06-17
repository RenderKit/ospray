// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "texture/Texture.ih"
#include "texture/Texture2D.ih"
#include "texture/TextureVolume.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f Texture_dispatch_get(
    const uniform Texture *uniform self, const DifferentialGeometry &dg)
{
  switch (self->type) {
  case TEXTURE_TYPE_2D:
    return Texture2D_get(self, dg);
#ifdef OSPRAY_ENABLE_VOLUMES
  case TEXTURE_TYPE_VOLUME:
    return TextureVolume_get(self, dg);
#endif
  default:
#ifndef OSPRAY_TARGET_SYCL
    return self->get(self, dg);
#else
    break;
#endif
  }
  return make_vec4f(0.f);
}

SYCL_EXTERNAL vec3f Texture_dispatch_getNormal(
    const uniform Texture *const uniform self, const DifferentialGeometry &dg)
{
  switch (self->type) {
  case TEXTURE_TYPE_2D:
    return Texture2D_getNormal(self, dg);
#ifdef OSPRAY_ENABLE_VOLUMES
  case TEXTURE_TYPE_VOLUME:
    return TextureVolume_getNormal(self, dg);
#endif
  default:
#ifndef OSPRAY_TARGET_SYCL
    return self->getNormal(self, dg);
#else
    break;
#endif
  }
  return make_vec3f(0.f);
}

OSPRAY_END_ISPC_NAMESPACE
