#include "TransferFunction.ih"

export void TransferFunction_PrecomputeMinMaxValues(void *uniform _transferFunction)
{
  uniform TransferFunction *uniform transferFunction = (uniform TransferFunction *uniform)_transferFunction;

  for(int i = 0; i < PRECOMPUTED_VALUES_DIM; i++) {
    float value = transferFunction->getAlphaForValue(transferFunction, i / ((float)PRECOMPUTED_VALUES_DIM - 1));
    transferFunction->precomputedMinMaxInRange[i][i] = make_vec2f(value,value);
  }

  for(int i = 0; i < PRECOMPUTED_VALUES_DIM; i++) { //from
    for(int j = i+1; j < PRECOMPUTED_VALUES_DIM; j++) { //to
      float currentValue = transferFunction->getAlphaForValue(transferFunction, j / ((float)PRECOMPUTED_VALUES_DIM - 1));
      transferFunction->precomputedMinMaxInRange[i][j] = make_vec2f(min(transferFunction->precomputedMinMaxInRange[i][j-1].x, currentValue),
								    max(transferFunction->precomputedMinMaxInRange[i][j-1].y, currentValue));
    }
  }
}
