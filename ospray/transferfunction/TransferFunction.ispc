#include "ospray/transferfunction/TransferFunction.ih"

export void TransferFunction_setValueRange(void *uniform pointer, 
                                           const uniform vec2f &value) 
{
  //! Cast to the actual type.
  TransferFunction *uniform transferFunction = (TransferFunction *uniform) pointer;

  //! Set the transfer function value range.
  transferFunction->valueRange = value;
}
