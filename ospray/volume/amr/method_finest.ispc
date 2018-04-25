// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "CellRef.ih"
#include "AMRVolume.ih"
#include "DualCell.ih"

varying float AMR_finest(void *uniform _self, const varying vec3f &P)
{
  const AMRVolume *uniform self = (AMRVolume *)_self;
  const AMR *uniform amr = &self->amr;

  vec3f lP;  //local amr space
  self->transformWorldToLocal(self, P, lP);

  DualCell D;
  initDualCell(D,lP,*amr->finestLevel);
  findDualCell(amr,D);
  return lerp(D);
}

varying float AMR_finestLevel(void *uniform _self, const varying vec3f &P)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  AMR *uniform amr = &self->amr;

  vec3f lP;  //local amr space
  self->transformWorldToLocal(self, P, lP);

  const CellRef C = findLeafCell(amr,lP);
  float width = C.width;
  if (isnan(width))
    width = 1.0f;
  return width;
}

export void AMR_install_finest(void *uniform _self)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  self->super.sample = AMR_finest;
  self->computeSampleLevel = AMR_finestLevel;
}
