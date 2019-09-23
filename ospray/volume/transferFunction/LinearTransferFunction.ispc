// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

export void *uniform LinearTransferFunction_create()
{
  LinearTransferFunction *uniform self = uniform new LinearTransferFunction;

  TransferFunction_Constructor(&self->super, LinearTransferFunction_get);
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
}
