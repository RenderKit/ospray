// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Data.ih"
// c++ shared
#include "LinearTransferFunctionShared.h"

vec4f LinearTransferFunction_get(const void *uniform _self, float value)
{
  const LinearTransferFunction *uniform self =
      (const LinearTransferFunction *uniform)_self;

  // remap to [0.0, 1.0]
  value = (value - self->super.valueRange.lower)
      * rcp(box_size(self->super.valueRange));
  // clamp to [0.0, 1.0)
  const uniform float nextBefore1 = 0x1.fffffep-1f;
  value = clamp(value, 0.0f, nextBefore1);

  const uniform int maxIdxC = self->color.numItems - 1;
  const float idxCf = value * maxIdxC;
  const float fracC = frac(idxCf);
  const int idxC = idxCf;

  const vec3f col = lerp(fracC,
      get_vec3f(self->color, idxC),
      get_vec3f(self->color, min(maxIdxC, idxC + 1)));

  const uniform int maxIdxO = self->opacity.numItems - 1;
  const float idxOf = value * maxIdxO;
  const float fracO = frac(idxOf);
  const int idxO = idxOf;

  const float opacity = lerp(fracO,
      get_float(self->opacity, idxO),
      get_float(self->opacity, min(maxIdxO, idxO + 1)));

  return make_vec4f(col, opacity);
}

float LinearTransferFunction_getMaxOpacity(
    const void *uniform _self, const range1f &valueRange)
{
  uniform LinearTransferFunction *uniform self =
      (uniform LinearTransferFunction * uniform) _self;
  const uniform int maxDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;
  const uniform float denom =
      self->super.valueRange.upper - self->super.valueRange.lower;
  const varying int i =
      floor(clamp((valueRange.lower - self->super.valueRange.lower) / denom)
          * maxDim);
  const varying int j =
      ceil(clamp((valueRange.upper - self->super.valueRange.lower) / denom)
          * maxDim);
  return self->maxOpacityInRange[min(i, maxDim)][min(j, maxDim)];
}

export void *uniform LinearTransferFunction_get_addr()
{
  return LinearTransferFunction_get;
}

export void *uniform LinearTransferFunction_getMaxOpacity_addr()
{
  return LinearTransferFunction_getMaxOpacity;
}
