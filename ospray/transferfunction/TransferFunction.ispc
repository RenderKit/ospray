#include "TransferFunction.ih"

export void TransferFunction_setValueRange(void *uniform _transferFunction,
					   uniform float minValue,
					   uniform float maxValue)
{
  uniform TransferFunction *uniform transferFunction = (uniform TransferFunction *uniform)_transferFunction;
  
  transferFunction->valueRange = make_vec2f(minValue,maxValue);
}
