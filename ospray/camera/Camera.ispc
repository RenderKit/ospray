// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"

export void Camera_set(void *uniform _self,
    const uniform vec3f &pos,
    const uniform linear3f &frame,
    const uniform float nearClip,
    const uniform vec2f &imageStart,
    const uniform vec2f &imageEnd,
    const uniform box1f &shutter)
{
  uniform Camera *uniform self = (uniform Camera * uniform) _self;
  self->org = pos;
  self->frame = frame;
  self->nearClip = nearClip;
  self->subImage.lower = imageStart;
  self->subImage.upper = imageEnd;
  self->shutter = shutter;
}
