// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/volume/Volume.ih"

void Volume_Constructor(Volume *uniform volume)
{
  // default for upperBound; should be set to correct value by derived volume.
  volume->upperBound = make_vec3f(1.f);

  // other defaults are set during Volume::updateEditableParameters().
}

export void Volume_setGammaCorrection(void *uniform _self, const uniform vec2f &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->gammaCorrection = value;
}

export void Volume_setGradientShadingEnabled(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->gradientShadingEnabled = value;
}

export void Volume_setNumIsovalues(void *uniform _self, const uniform int value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->numIsovalues = value;
}

export void Volume_setIsovalues(void *uniform _self, uniform float *uniform values)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->isovalues = values;
}

export void Volume_setSamplingRate(void *uniform _self, const uniform float value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->samplingRate = value;
}

export void Volume_setTransferFunction(void *uniform _self, void *uniform value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->transferFunction = (TransferFunction *uniform) value;
}

export void Volume_setVolumeClippingBox(void *uniform _self, const uniform box3f &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->volumeClippingBox = value;
}
