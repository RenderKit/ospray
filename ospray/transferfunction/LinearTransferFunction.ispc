// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "ospray/transferfunction/LinearTransferFunction.ih"

inline varying vec3f 
LinearTransferFunction_getColorForValue(const void *uniform pointer, 
                                        varying float value) 
{
  // Cast to the actual TransferFunction subtype.
  const LinearTransferFunction *uniform transferFunction = (const LinearTransferFunction *uniform) pointer;

  // No color values may be available.
  if (transferFunction->colorValueCount == 0) return(make_vec3f(1.0f));

  // Clamp the value to the lower bound of the value range.
  if (value <= transferFunction->inherited.valueRange.x) return(transferFunction->colorValues[0]);

  // Clamp the value to the upper bound of the value range.
  if (value >= transferFunction->inherited.valueRange.y) return(transferFunction->colorValues[transferFunction->colorValueCount - 1]);

  // Map the value into the range [0.0, 1.0].
  value = (value - transferFunction->inherited.valueRange.x) / (transferFunction->inherited.valueRange.y - transferFunction->inherited.valueRange.x) * (transferFunction->colorValueCount - 1.0f);

  // Compute the color index and fractional offset.
  int index = floor(value);  float remainder = value - index;

  // The interpolated color.
  return((1.0f - remainder) * transferFunction->colorValues[index] + remainder * transferFunction->colorValues[min(index + 1, transferFunction->colorValueCount - 1)]);

}

uniform vec2f 
LinearTransferFunction_getMinMaxOpacityInRange(void *uniform pointer, 
                                               const uniform vec2f &range) 
{
  uniform LinearTransferFunction *uniform transferFunction = (uniform LinearTransferFunction *uniform) pointer;
  const uniform int maxDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;
  const uniform float denom = transferFunction->inherited.valueRange.y - transferFunction->inherited.valueRange.x;
  const uniform int i = floor(clamp((range.x - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  const uniform int j = ceil(clamp((range.y - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  return(transferFunction->minMaxOpacityInRange[i][j]);

}

varying float 
LinearTransferFunction_getMaxOpacityInRange(void *uniform pointer, 
                                            const varying vec2f &range) 
{
  uniform LinearTransferFunction *uniform transferFunction = (uniform LinearTransferFunction *uniform) pointer;
  const uniform int maxDim = PRECOMPUTED_OPACITY_SUBRANGE_COUNT - 1;
  const uniform float denom = transferFunction->inherited.valueRange.y - transferFunction->inherited.valueRange.x;
  const varying int i = floor(clamp((range.x - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  const varying int j = ceil(clamp((range.y - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  return(transferFunction->minMaxOpacityInRange[i][j].y);

}

inline varying float 
LinearTransferFunction_getOpacityForValue(const void *uniform pointer, 
                                          varying float value) 
{
  // Cast to the actual TransferFunction subtype.
  const LinearTransferFunction *uniform transferFunction = (const LinearTransferFunction *uniform) pointer;

  // No opacity values may be available.
  if (transferFunction->opacityValueCount == 0) return(1.0f);

  // Clamp the value to the lower bound of the value range.
  if (value <= transferFunction->inherited.valueRange.x) return(transferFunction->opacityValues[0]);

  // Clamp the value to the upper bound of the value range.
  if (value >= transferFunction->inherited.valueRange.y) return(transferFunction->opacityValues[transferFunction->opacityValueCount - 1]);

  // Map the value into the range [0.0, 1.0].
  value = (value - transferFunction->inherited.valueRange.x) / (transferFunction->inherited.valueRange.y - transferFunction->inherited.valueRange.x) * (transferFunction->opacityValueCount - 1.0f);

  // Compute the opacity index and fractional offset.
  int index = floor(value);  float remainder = value - index;

  // The interpolated opacity.
  return((1.0f - remainder) * transferFunction->opacityValues[index] + remainder * transferFunction->opacityValues[min(index + 1, transferFunction->opacityValueCount - 1)]);

}

void LinearTransferFunction_precomputeMinMaxOpacityRanges(void *uniform pointer) 
{
  uniform LinearTransferFunction *uniform transferFunction = (uniform LinearTransferFunction *uniform) pointer;

  // Compute the diagonal.
  for (int i=0 ; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT ; i++) {

    // Figure out the range of values in the array we are going to compare.
    const int checkRangeLow = transferFunction->opacityValueCount * (((float) i) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);
    const int checkRangeHigh = transferFunction->opacityValueCount * (((float) i + 1) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);

    vec2f range = make_vec2f(transferFunction->opacityValues[checkRangeLow], transferFunction->opacityValues[checkRangeLow]);
    for (int opacityIDX = checkRangeLow + 1 ; opacityIDX < checkRangeHigh ; opacityIDX++)
      range = make_vec2f(min(range.x, transferFunction->opacityValues[opacityIDX]), max(range.y, transferFunction->opacityValues[opacityIDX]));

    transferFunction->minMaxOpacityInRange[i][i] = range;

  }

  // Fill out each column from the diagonal up.
  for (int i=0 ; i < PRECOMPUTED_OPACITY_SUBRANGE_COUNT ; i++) {
    for (int j = i + 1 ; j < PRECOMPUTED_OPACITY_SUBRANGE_COUNT ; j++) {

      // Figure out the range of values in the array we are going to compare.
      const int checkRangeLow = transferFunction->opacityValueCount * (((float) i) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);
      const int checkRangeHigh = transferFunction->opacityValueCount * (((float) j + 1) / PRECOMPUTED_OPACITY_SUBRANGE_COUNT);

      vec2f range = transferFunction->minMaxOpacityInRange[i][i];
      for (int opacityIDX = checkRangeLow + 1 ; opacityIDX < checkRangeHigh ; opacityIDX++)
        range = make_vec2f(min(range.x, transferFunction->opacityValues[opacityIDX]), max(range.y, transferFunction->opacityValues[opacityIDX]));

      transferFunction->minMaxOpacityInRange[i][j] = range;

    }

  }

}

export void *uniform LinearTransferFunction_createInstance() 
{
  // The transfer function.
  LinearTransferFunction *uniform transferFunction = uniform new uniform LinearTransferFunction;

  // Function to get the interpolated color for a given value.
  transferFunction->inherited.getColorForValue = LinearTransferFunction_getColorForValue;

  // Function to get the interpolated opacity for a given value.
  transferFunction->inherited.getOpacityForValue = LinearTransferFunction_getOpacityForValue;

  // Virtual function to look up the maximum opacity based on an input range.
  transferFunction->inherited.getMaxOpacityInRange = LinearTransferFunction_getMaxOpacityInRange;

  // Virtual function to look up the min/max opacity based on an input range.
  transferFunction->inherited.getMinMaxOpacityInRange = LinearTransferFunction_getMinMaxOpacityInRange;

  // Transfer function colors and count.
  transferFunction->colorValues = NULL;  transferFunction->colorValueCount = 0;

  // Transfer function opacity values and count.
  transferFunction->opacityValues = NULL;  transferFunction->opacityValueCount = 0;

  // The default transfer function value range.
  transferFunction->inherited.valueRange = make_vec2f(0.0f, 1.0f);

  // Freshly baked transfer function.
  return(transferFunction);

}

export void LinearTransferFunction_destroy(void *uniform pointer) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

  // Free memory for the color values.
  if (transferFunction->colorValues != NULL) delete[] transferFunction->colorValues;

  // Free memory for the opacity values.
  if (transferFunction->opacityValues != NULL) delete[] transferFunction->opacityValues;

  // Free memory for the container.
  delete transferFunction;

}

export void LinearTransferFunction_setColorValues(void *uniform pointer, 
                                                  const uniform size_t &count, 
                                                  vec3f *uniform source) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

  // Free memory for any existing color values.
  if (transferFunction->colorValues != NULL) delete[] transferFunction->colorValues;

  // Allocate memory for the incoming color values.
  transferFunction->colorValueCount = count;  transferFunction->colorValues = uniform new uniform vec3f[count];

  // Copy the color values into the transfer function.
  for (size_t i=0 ; i < count ; i++) transferFunction->colorValues[i] = source[i];

}

export void LinearTransferFunction_setOpacityValues(void *uniform pointer, 
                                                    const uniform size_t &count, 
                                                    float *uniform source) 
{
  // Cast to the actual TransferFunction subtype.
  LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

  // Free memory for any existing opacity values.
  if (transferFunction->opacityValues != NULL) delete[] transferFunction->opacityValues;

  // Allocate memory for the incoming opacity values.
  transferFunction->opacityValueCount = count;  transferFunction->opacityValues = uniform new uniform float[count];

  // Copy the opacity values into the transfer function.
  for (size_t i=0 ; i < count ; i++) transferFunction->opacityValues[i] = source[i];

  // Precompute the min / max opacity ranges.
  LinearTransferFunction_precomputeMinMaxOpacityRanges(pointer);

}

