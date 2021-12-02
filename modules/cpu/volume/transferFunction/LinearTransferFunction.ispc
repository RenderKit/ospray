// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.ih"

inline vec4f LinearTransferFunction_get(const void *uniform _self, float value)
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

inline float LinearTransferFunction_getMaxOpacity(
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

inline void LinearTransferFunction_precomputeMaxOpacityRanges(
    void *uniform _self)
{
  uniform LinearTransferFunction *uniform self =
      (uniform LinearTransferFunction * uniform) _self;

  const uniform int maxOpacityDim = self->opacity.numItems - 1;
  const uniform int maxPrecomputedDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;

  // compute the diagonal
  for (uniform int i = 0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {
    // figure out the range of array indices we are going to compare; this is a
    // conservative range of feasible indices that may be used to lookup
    // opacities for any data value within the value range corresponding to [i,
    // i].
    const uniform int checkRangeLow =
        floor(maxOpacityDim * (float)i / maxPrecomputedDim);
    const uniform int checkRangeHigh =
        ceil(maxOpacityDim * (float)i / maxPrecomputedDim);

    uniform float maxOpacity = get_float(self->opacity, checkRangeLow);
    for (uniform int opacityIDX = checkRangeLow; opacityIDX <= checkRangeHigh;
         opacityIDX++)
      maxOpacity = max(maxOpacity, get_float(self->opacity, opacityIDX));

    self->maxOpacityInRange[i][i] = maxOpacity;
  }

  // fill out each column from the diagonal up
  for (uniform int i = 0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {
    for (uniform int j = i + 1; j < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; j++) {
      // figure out the range of array indices we are going to compare; this is
      // a conservative range of feasible indices that may be used to lookup
      // opacities for any data value within the value range corresponding to
      // [i, j].
      const uniform int checkRangeLow =
          floor(maxOpacityDim * (float)i / maxPrecomputedDim);
      const uniform int checkRangeHigh =
          ceil(maxOpacityDim * (float)j / maxPrecomputedDim);

      uniform float maxOpacity = self->maxOpacityInRange[i][i];
      for (uniform int opacityIDX = checkRangeLow; opacityIDX <= checkRangeHigh;
           opacityIDX++)
        maxOpacity = max(maxOpacity, get_float(self->opacity, opacityIDX));

      self->maxOpacityInRange[i][j] = maxOpacity;
    }
  }
}

export void *uniform LinearTransferFunction_create()
{
  LinearTransferFunction *uniform self = uniform new LinearTransferFunction;

  TransferFunction_Constructor(&self->super,
      LinearTransferFunction_get,
      LinearTransferFunction_getMaxOpacity);
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->opacity);

  return self;
}

export void LinearTransferFunction_set(void *uniform _self,
    const Data1D *uniform color,
    const Data1D *uniform opacity)
{
  LinearTransferFunction *uniform self =
      (LinearTransferFunction * uniform) _self;

  self->color = *color;
  self->opacity = *opacity;

  LinearTransferFunction_precomputeMaxOpacityRanges(_self);
}
