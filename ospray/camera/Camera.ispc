// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"

export void Camera_set(void *uniform _self,
    const uniform float nearClip,
    const uniform vec2f &imageStart,
    const uniform vec2f &imageEnd,
    const uniform box1f &shutter,
    const uniform bool globalShutter,
    const uniform float rollingShutterDuration,
    const uniform bool rollingShutterHorizontal,
    const uniform bool motionBlur,
    RTCGeometry geom)
{
  uniform Camera *uniform self = (uniform Camera * uniform) _self;
  self->nearClip = nearClip;
  self->subImage.lower = imageStart;
  self->subImage.upper = imageEnd;
  self->shutter = shutter;
  self->motionBlur = motionBlur;
  self->geom = geom;
  self->globalShutter = globalShutter;
  self->rollingShutterHorizontal = rollingShutterHorizontal;
  self->rollingShutterDuration = rollingShutterDuration;
}
