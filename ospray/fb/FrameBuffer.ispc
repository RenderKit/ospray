// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameBuffer.ih"

void FrameBuffer_Constructor(FrameBuffer *uniform self, void *uniform cClassPtr)
{
  self->cClassPtr = cClassPtr;
  self->size.x = 0;
  self->size.y = 0;
  self->rcpSize.x = 0.f;
  self->rcpSize.y = 0.f;
  self->colorBufferFormat = ColorBufferFormat_NONE;
}

void FrameBuffer_set(FrameBuffer *uniform self,
    const uniform uint32 size_x,
    const uniform uint32 size_y,
    uniform int32 colorBufferFormat)
{
  self->size.x = size_x;
  self->size.y = size_y;
  self->rcpSize.x = 1.f / size_x;
  self->rcpSize.y = 1.f / size_y;
  self->colorBufferFormat =
      (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
}

export void FrameBuffer_set_frameID(void *uniform _self, uniform int32 frameID)
{
  uniform FrameBuffer *uniform self = (uniform FrameBuffer * uniform) _self;
  self->frameID = frameID;
}
