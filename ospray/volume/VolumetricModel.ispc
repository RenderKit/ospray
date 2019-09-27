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

// ospray
#include "VolumetricModel.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

export void *uniform VolumetricModel_create(void *uniform cppE,
                                            void *uniform volume)
{
  VolumetricModel *uniform self = uniform new VolumetricModel;

  self->volume = (Volume * uniform) volume;

  return self;
}

export void VolumetricModel_set(void *uniform _self,
                                uniform float samplingRate,
                                void *uniform transferFunction,
                                const uniform box3f &bounds,
                                uniform float densityScale,
                                uniform float anisotropy)
{
  VolumetricModel *uniform self = (VolumetricModel * uniform) _self;

  self->samplingRate     = samplingRate;
  self->samplingStep     = self->volume->samplingStep;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->boundingBox      = bounds;
  self->densityScale     = densityScale;
  self->anisotropy       = anisotropy;
}

export void VolumetricModel_set_valueSelector(void *uniform _self,
                                              void *uniform valueSelector)
{
  VolumetricModel *uniform self = (VolumetricModel * uniform) _self;
  self->vklValueSelector = (VKLValueSelector)valueSelector;
}