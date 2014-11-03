/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/transferfunction/LinearTransferFunction.ih"

export void *uniform LinearTransferFunction_createInstance() {

    // The transfer function.
    LinearTransferFunction *uniform transferFunction = uniform new uniform LinearTransferFunction;

    // Function to get the interpolated color for a given value.
    transferFunction->inherited.getColorForValue = LinearTransferFunction_getColorForValue;

    // Function to get the interpolated opacity for a given value.
    transferFunction->inherited.getAlphaForValue = LinearTransferFunction_getAlphaForValue;
    
    // Virtual function to precompute the min/max alpha ranges of a transfer function
    transferFunction->inherited.precomputeMinMaxAlphaRanges = LinearTransferFunction_precomputeMinMaxAlphaRanges;

    // Virtual function to look up the min/max alpha based on an input range
    transferFunction->inherited.getMinMaxAlphaInRange = LinearTransferFunction_getMinMaxAlphaInRange;

    // Transfer function colors, count, and range.
    transferFunction->colors = NULL;  transferFunction->colorCount = 0;  transferFunction->colorRange = make_vec2f(0.0f, 1.0f);

    // Transfer function alpha values, count, and range.
    transferFunction->alphas = NULL;  transferFunction->alphaCount = 0;  transferFunction->alphaRange = make_vec2f(0.0f, 1.0f);

    // Freshly baked transfer function.
    return(transferFunction);

}

export void LinearTransferFunction_destroy(void *uniform pointer) {

    // Cast to the actual TransferFunction subtype.
    LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

    // Free memory for the color values.
    if (transferFunction->colors != NULL) delete[] transferFunction->colors;

    // Free memory for the opacity values.
    if (transferFunction->alphas != NULL) delete[] transferFunction->alphas;

    // Free memory for the struct.
    delete transferFunction;

}

inline varying float LinearTransferFunction_getAlphaForValue(const void *uniform pointer, varying float value) {

    // Cast to the actual TransferFunction subtype.
    const LinearTransferFunction *uniform transferFunction = (const LinearTransferFunction *uniform) pointer;

    // No opacity values may be available.
    if (transferFunction->alphaCount == 0) return(1.0f);

    // Clamp the value to the lower bound of the opacity value range.
    if (value <= transferFunction->alphaRange.x) return(transferFunction->alphas[0]);

    // Clamp the value to the upper bound of the opacity value range.
    if (value >= transferFunction->alphaRange.y) return(transferFunction->alphas[transferFunction->alphaCount - 1]);

    // Map the value into the range [0.0, 1.0].
    value = (value - transferFunction->alphaRange.x) / (transferFunction->alphaRange.y - transferFunction->alphaRange.x) * (transferFunction->alphaCount - 1.0f);

    // Compute the opacity index and fractional offset.
    int index = floor(value);  float remainder = value - index;

    // The interpolated opacity.
    return((1.0f - remainder) * transferFunction->alphas[index] + remainder * transferFunction->alphas[min(index + 1, transferFunction->alphaCount - 1)]);
    
}

inline varying vec3f LinearTransferFunction_getColorForValue(const void *uniform pointer, varying float value) {

    // Cast to the actual TransferFunction subtype.
    const LinearTransferFunction *uniform transferFunction = (const LinearTransferFunction *uniform) pointer;

    // No color values may be available.
    if (transferFunction->colorCount == 0) return(make_vec3f(1.0f));

    // Clamp the value to the lower bound of the color value range.
    if (value <= transferFunction->colorRange.x) return(transferFunction->colors[0]);

    // Clamp the value to the upper bound of the color value range.
    if (value >= transferFunction->colorRange.y) return(transferFunction->colors[transferFunction->colorCount - 1]);

    // Map the value into the range [0.0, 1.0].
    value = (value - transferFunction->colorRange.x) / (transferFunction->colorRange.y - transferFunction->colorRange.x) * (transferFunction->colorCount - 1.0f);

    // Compute the color index and fractional offset.
    int index = floor(value);  float remainder = value - index;

    // The interpolated color.
    return((1.0f - remainder) * transferFunction->colors[index] + remainder * transferFunction->colors[min(index + 1, transferFunction->colorCount - 1)]);

}

export void LinearTransferFunction_setColorRange(void *uniform pointer, const uniform float minimum, const uniform float maximum) {

    // Cast to the actual TransferFunction subtype.
    LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

    // Set the color value range.
    transferFunction->colorRange = make_vec2f(minimum, maximum);

}

export void LinearTransferFunction_setColorValues(void *uniform pointer, const uniform size_t &count, vec3f *uniform source) {

    // Cast to the actual TransferFunction subtype.
    LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

    // Free memory for any existing color values.
    if (transferFunction->colors != NULL) delete[] transferFunction->colors;

    // Allocate memory for the incoming color values.
    transferFunction->colorCount = count;  transferFunction->colors = uniform new uniform vec3f[count];

    // Copy the color values into the transfer function.
    for (size_t i=0 ; i < count ; i++) transferFunction->colors[i] = source[i];

}

export void LinearTransferFunction_setAlphaRange(void *uniform pointer, const uniform float minimum, const uniform float maximum) {

    // Cast to the actual TransferFunction subtype.
    LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

    // Set the opacity value range.
    transferFunction->alphaRange = make_vec2f(minimum, maximum);

}

export void LinearTransferFunction_setAlphaValues(void *uniform pointer, const uniform size_t &count, float *uniform source) {

    // Cast to the actual TransferFunction subtype.
    LinearTransferFunction *uniform transferFunction = (LinearTransferFunction *uniform) pointer;

    // Free memory for any existing opacity values.
    if (transferFunction->alphas != NULL) delete[] transferFunction->alphas;

    // Allocate memory for the incoming opacity values.
    transferFunction->alphaCount = count;  transferFunction->alphas = uniform new uniform float[count];

    // Copy the opacity values into the transfer function.
    for (size_t i=0 ; i < count ; i++) transferFunction->alphas[i] = source[i];

    // Precompute the min / max alpha ranges.
    transferFunction->inherited.precomputeMinMaxAlphaRanges(transferFunction);

}

void LinearTransferFunction_precomputeMinMaxAlphaRanges(void *uniform pointer) {

  uniform LinearTransferFunction *uniform transferFunction = (uniform LinearTransferFunction *uniform)pointer;

  //Compute the diagonal
  for (int i = 0; i < PRECOMPUTED_VALUES_DIM; i++) {
    //Figure out the range of values in the array we are going to compare
    const int checkRangeLow = transferFunction->alphaCount * (((float)i) / PRECOMPUTED_VALUES_DIM);
    const int checkRangeHigh = transferFunction->alphaCount * (((float)i+1) / PRECOMPUTED_VALUES_DIM);
    
    vec2f range = make_vec2f(transferFunction->alphas[checkRangeLow], transferFunction->alphas[checkRangeLow]);
    for (int alphaIDX = checkRangeLow+1; alphaIDX < checkRangeHigh; alphaIDX++) {
      range = make_vec2f(min(range.x, transferFunction->alphas[alphaIDX]),
			 max(range.y, transferFunction->alphas[alphaIDX]));
    }
    
    transferFunction->precomputedMinMaxInRange[i][i] = range;
  }
  
  //Fill out each column from the diagonal up
  for (int i = 0; i < PRECOMPUTED_VALUES_DIM; i++) {
    for (int j = i+1; j < PRECOMPUTED_VALUES_DIM; j++) {
      //Figure out the range of values in the array we are going to compare
      const int checkRangeLow = transferFunction->alphaCount * (((float)i) / PRECOMPUTED_VALUES_DIM);
      const int checkRangeHigh = transferFunction->alphaCount * (((float)j+1) / PRECOMPUTED_VALUES_DIM);

      vec2f range = transferFunction->precomputedMinMaxInRange[i][i];
      for (int alphaIDX = checkRangeLow+1; alphaIDX < checkRangeHigh; alphaIDX++) {
	range = make_vec2f(min(range.x, transferFunction->alphas[alphaIDX]),
			   max(range.y, transferFunction->alphas[alphaIDX]));
      }
    
      transferFunction->precomputedMinMaxInRange[i][j] = range;
    }
  }

}

uniform vec2f LinearTransferFunction_getMinMaxAlphaInRange(void *uniform pointer, uniform float rangeLow, uniform float rangeHigh) 
{
  uniform LinearTransferFunction *uniform transferFunction = (uniform LinearTransferFunction *uniform)pointer;
  const uniform int maxDim = PRECOMPUTED_VALUES_DIM - 1;
  const uniform float denom = transferFunction->inherited.valueRange.y - transferFunction->inherited.valueRange.x;
  const uniform int i = floor(clamp((rangeLow - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  const uniform int j = ceil(clamp((rangeHigh - transferFunction->inherited.valueRange.x) / denom) * maxDim);
  return transferFunction->precomputedMinMaxInRange[i][j];
}

