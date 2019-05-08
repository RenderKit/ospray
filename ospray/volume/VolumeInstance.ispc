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
#include "VolumeInstance.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

export void *uniform VolumeInstance_create(void *uniform cppE,
                                           void *uniform volume)
{
  VolumeInstance *uniform self = uniform new VolumeInstance;

  self->volume = (Volume * uniform) volume;

  return self;
}

export void VolumeInstance_set(void *uniform _self,
                               uniform bool preIntegration,
                               uniform bool adaptiveSampling,
                               uniform float adaptiveScalar,
                               uniform float adaptiveMaxSamplingRate,
                               uniform float adaptiveBacktrack,
                               uniform float samplingRate,
                               void *uniform transferFunction,
                               const uniform box3f &clippingBox,
                               const uniform box3f &instanceBounds,
                               const uniform AffineSpace3f &xfm,
                               const uniform AffineSpace3f &rcp_xfm)
{
  VolumeInstance *uniform self = (VolumeInstance * uniform) _self;

  self->preIntegration          = preIntegration;
  self->adaptiveSampling        = adaptiveSampling;
  self->adaptiveScalar          = adaptiveScalar;
  self->adaptiveMaxSamplingRate = adaptiveMaxSamplingRate;
  self->adaptiveBacktrack       = adaptiveBacktrack;
  self->samplingRate            = samplingRate;
  self->transferFunction        = (TransferFunction * uniform) transferFunction;
  self->volumeClippingBox       = clippingBox;
  self->boundingBox             = instanceBounds;
  self->xfm                     = xfm;
  self->rcp_xfm                 = rcp_xfm;
}
