// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureParam.ih"

export void TextureParam_set(TextureParam *uniform self,
    void *uniform ptr,
    uniform TransformFlags transformFlags,
    const uniform affine2f &xform2f,
    const uniform affine3f &xform3f)
{
  self->ptr = (Texture2D * uniform) ptr;
  self->transformFlags = transformFlags;
  self->xform2f = xform2f;
  self->xform3f = xform3f;
}
