// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "transferFunction/LinearTransferFunction.ih"

inline varying vec3f 
LinearTransferFunction_getColorForValue(const void *uniform _self, 
                                        varying float value) 
{
  // Return (0,0,0) for NaN values.
  if (isnan(value)) 
    return make_vec3f(0.0f);

  // Cast to the actual TransferFunction subtype.
  const LinearTransferFunction *uniform self
    = (const LinearTransferFunction *uniform) _self;

  // No color values may be available.
  if (self->colorValueCount == 0) 
    return make_vec3f(1.0f);

  // Clamp the value to the lower bound of the value range.
  if (value <= self->super.valueRange.x) 
    return self->colorValues[0];

  // Clamp the value to the upper bound of the value range.
  if (value >= self->super.valueRange.y) 
    return self->colorValues[self->colorValueCount - 1];

  // Map the value into the range [0.0, 1.0].
  value
    = (value - self->super.valueRange.x) 
    / (self->super.valueRange.y - self->super.valueRange.x) 
    * (self->colorValueCount - 1.0f);

  // Compute the color index and fractional offset.
  int   index     = floor(value);  
  float remainder = value - index;

  // The interpolated color.
  return
    ((1.0f - remainder) * self->colorValues[index] 
     + remainder        * self->colorValues[min(index + 1, self->colorValueCount - 1)]);
}

uniform vec2f 
LinearTransferFunction_getMinMaxOpacityInRange(void *uniform _self, 
                                               const uniform vec2f &range) 
{
  uniform LinearTransferFunction *uniform self = (uniform LinearTransferFunction *uniform) _self;
  const uniform int   maxDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;
  const uniform float denom  = self->super.valueRange.y - self->super.valueRange.x;
  const uniform int   i      = floor(clamp((range.x - self->super.valueRange.x) / denom) * maxDim);
  const uniform int   j      = ceil(clamp((range.y - self->super.valueRange.x) / denom) * maxDim);
  return self->minMaxOpacityInRange[min(i,maxDim)][min(j,maxDim)];
}

varying float 
LinearTransferFunction_getMaxOpacityInRange(void *uniform _self, 
                                            const varying vec2f &range) 
{
  uniform LinearTransferFunction *uniform self = (uniform LinearTransferFunction *uniform) _self;
  const uniform int maxDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;
  const uniform float denom = self->super.valueRange.y - self->super.valueRange.x;
  const varying int i = floor(clamp((range.x - self->super.valueRange.x) / denom) * maxDim);
  const varying int j = ceil(clamp((range.y - self->super.valueRange.x) / denom) * maxDim);
  return self->minMaxOpacityInRange[min(i,maxDim)][min(j,maxDim)].y;

}

inline varying float 
LinearTransferFunction_getOpacityForValue(const void *uniform _self, 
                                          varying float value) 
{
  // Return 0 for NaN values.
  if (isnan(value)) return 0.0f;

  // Cast to the actual TransferFunction subtype.
  const LinearTransferFunction *uniform self
    = (const LinearTransferFunction *uniform) _self;

  // No opacity values may be available.
  if (self->opacityValueCount == 0) 
    return 1.0f;

  // Clamp the value to the lower bound of the value range.
  if (value <= self->super.valueRange.x) 
    return self->opacityValues[0];

  // Clamp the value to the upper bound of the value range.
  if (value >= self->super.valueRange.y) 
    return self->opacityValues[self->opacityValueCount - 1];

  // Map the value into the range [0.0, numValues).
  const float remapped
    = (value - self->super.valueRange.x)
    / (self->super.valueRange.y - self->super.valueRange.x) 
    * (self->opacityValueCount - 1.0f);

  // Compute the opacity index and fractional offset.
  int index = floor(remapped);  
  float remainder = remapped - index;

  // The interpolated opacity.
  float ret =
    (1.0f - remainder) * self->opacityValues[index] 
    + remainder * self->opacityValues[min(index + 1, self->opacityValueCount - 1)];
  return ret;
}

void LinearTransferFunction_precomputeMinMaxOpacityRanges(void *uniform _self) 
{
  uniform LinearTransferFunction *uniform self
    = (uniform LinearTransferFunction *uniform) _self;

  // Compute the diagonal.
  for (uniform int i=0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {

    // Figure out the range of values in the array we are going to compare.
    const uniform int checkRangeLow
      = self->opacityValueCount * (((float) i) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);
    const uniform int checkRangeHigh
      = self->opacityValueCount * (((float)i + 1) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);

    uniform vec2f range = make_vec2f(self->opacityValues[checkRangeLow]);
    for (uniform int opacityIDX = checkRangeLow + 1; 
         opacityIDX < checkRangeHigh; 
         opacityIDX++)
      range = make_vec2f(min(range.x, self->opacityValues[opacityIDX]), 
                         max(range.y, self->opacityValues[opacityIDX]));

    self->minMaxOpacityInRange[i][i] = range;
  }

  // Fill out each column from the diagonal up.
  for (uniform int i=0; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; i++) {
    for (uniform int j = i + 1; j < PRECOMPUTED_OPACITY_SUBRANGE_COUNT; j++) {

      // Figure out the range of values in the array we are going to compare.
      const uniform int checkRangeLow = self->opacityValueCount * (((float) i) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);
      const uniform int checkRangeHigh = self->opacityValueCount * (((float) j + 1) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);

      uniform vec2f range = self->minMaxOpacityInRange[i][i];
      for (uniform int opacityIDX = checkRangeLow + 1; opacityIDX < checkRangeHigh; opacityIDX++)
        range = make_vec2f(min(range.x, self->opacityValues[opacityIDX]), 
                           max(range.y, self->opacityValues[opacityIDX]));

      self->minMaxOpacityInRange[i][j] = range;

    }

  }

}

export void *uniform LinearTransferFunction_createInstance() 
{
  // The transfer function.
  LinearTransferFunction *uniform self = uniform new uniform LinearTransferFunction;

  // Function to get the interpolated color for a given value.
  self->super.getColorForValue = LinearTransferFunction_getColorForValue;

  // Function to get the interpolated opacity for a given value.
  self->super.getOpacityForValue = LinearTransferFunction_getOpacityForValue;

  // Virtual function to look up the maximum opacity based on an input range.
  self->super.getMaxOpacityInRange = LinearTransferFunction_getMaxOpacityInRange;

  // Virtual function to look up the min/max opacity based on an input range.
  self->super.getMinMaxOpacityInRange = LinearTransferFunction_getMinMaxOpacityInRange;

  // Transfer function colors and count.
  self->colorValues = NULL;  
  self->colorValueCount = 0;

  // Transfer function opacity values and count.
  self->opacityValues = NULL;  
  self->opacityValueCount = 0;

  // The default transfer function value range.
  self->super.valueRange = make_vec2f(0.0f, 1.0f);

  // Freshly baked transfer function.
  return self;
}

export void LinearTransferFunction_destroy(void *uniform _self) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform self = (LinearTransferFunction *uniform) _self;

  // // Free memory for the color values.
  // if (self->colorValues != NULL) delete[] self->colorValues;

  // // Free memory for the opacity values.
  // if (self->opacityValues != NULL) delete[] self->opacityValues;

  // Container is deallocated by ~ManagedObject
}

export void LinearTransferFunction_setColorValues(void *uniform _self, 
                                                  const uniform size_t &count, 
                                                  vec3f *uniform source) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform self
    = (LinearTransferFunction *uniform) _self;

  // Free memory for any existing color values.
  if (self->colorValues != NULL) 
    delete[] self->colorValues;

  // Allocate memory for the incoming color values.
  self->colorValueCount = count;  
  self->colorValues = uniform new uniform vec3f[count];

  // Copy the color values into the transfer function.
  for (uniform size_t i=0; i < count; i++) 
    self->colorValues[i] = source[i];

}

export void LinearTransferFunction_setOpacityValues(void *uniform _self, 
                                                    const uniform size_t &count, 
                                                    float *uniform source) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform self
    = (LinearTransferFunction *uniform) _self;

  self->opacityValues = source;
  self->opacityValueCount = count;

  // // Free memory for any existing opacity values.
  // if (self->opacityValues != NULL) 
  //   delete[] self->opacityValues;

  // // Allocate memory for the incoming opacity values.
  // self->opacityValueCount = count;  
  // self->opacityValues = uniform new uniform float[count];

  // // Copy the opacity values into the transfer function.
  // for (uniform size_t i=0; i < count; i++) 
  //   selfy->opacityValues[i] = source[i];

  // Precompute the min / max opacity ranges.
  LinearTransferFunction_precomputeMinMaxOpacityRanges(_self);

}
