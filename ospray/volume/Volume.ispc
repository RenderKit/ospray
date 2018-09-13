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

#include "volume/Volume.ih"

void Volume_Constructor(Volume *uniform self,
                        /*! pointer to the c++-equivalent class of this entity */
                        void *uniform cppEquivalent
                        )
{
  self->cppEquivalent = cppEquivalent;

  // default sampling step; should be set to correct value by derived volume.
  self->samplingStep = 1.f;

  // default bounding box; should be set to correct value by derived volume.
  self->boundingBox = make_box3f(make_vec3f(0.f), make_vec3f(1.f));

  // other defaults are set during Volume::updateEditableParameters().
}

export void Volume_setGradientShadingEnabled(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->gradientShadingEnabled = value;
}

export void Volume_setPreIntegration(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->preIntegration = value;
}

export void Volume_setSingleShade(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->singleShade = value;
}

export void Volume_setAdaptiveSampling(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->adaptiveSampling = value;
}

export void Volume_setAdaptiveScalar(void *uniform _self, const uniform float &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->adaptiveScalar = value;
}

export void Volume_setAdaptiveMaxSamplingRate(void *uniform _self, const uniform float &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->adaptiveMaxSamplingRate = value;
}

export void Volume_setAdaptiveBacktrack(void *uniform _self, const uniform float &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->adaptiveBacktrack = value;
}

export void Volume_setSamplingRate(void *uniform _self, const uniform float value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->samplingRate = value;
}

export void Volume_setSpecular(void *uniform _self, const uniform vec3f &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->specular = value;
}

export void Volume_setNs(void *uniform _self, const uniform float &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->ns = value;
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

export void Volume_setAffineTransformations(void *uniform _self,
					    const uniform AffineSpace3f &xfm,
					    const uniform AffineSpace3f &rcp_xfm)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->xfm = xfm;
  self->rcp_xfm = rcp_xfm;
}

export void Volume_getBoundingBox(uniform box3f *uniform ret, void *uniform _self)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  *ret = self->boundingBox;
}

export void Volume_computeSamples(void *uniform _self,
                                  uniform float *uniform *uniform results,
                                  const uniform vec3f *uniform worldCoordinates,
                                  const uniform size_t &count)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;

  foreach (i=0 ... count) {
    vec3f c = worldCoordinates[i];
    float sample = self->sample(self, c);
    (*results)[i] = sample;
  }
}
