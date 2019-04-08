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

#include "volume/Volume.ih"

void Volume_Constructor(Volume *uniform self, void *uniform cppEquivalent)
{
  self->cppEquivalent = cppEquivalent;

  // default sampling step; should be set to correct value by derived volume.
  self->samplingStep = 1.f;

  // default bounding box; should be set to correct value by derived volume.
  self->boundingBox = make_box3f(make_vec3f(0.f), make_vec3f(1.f));
}

export void Volume_getBoundingBox(uniform box3f *uniform ret,
                                  void *uniform _self)
{
  uniform Volume *uniform self = (uniform Volume * uniform) _self;
  // If we have a clip box then the actual volume bounds are the intersection
  // of the volume bounds and the clip box
  cif(ne(self->volumeClippingBox.lower, self->volumeClippingBox.upper))
  {
    ret->lower = max(self->boundingBox.lower, self->volumeClippingBox.lower);
    ret->upper = min(self->boundingBox.upper, self->volumeClippingBox.upper);
  }
  else
  {
    *ret = self->boundingBox;
  }
}

export void Volume_set(void *uniform _self,
                       uniform bool gradientShading,
                       uniform bool preIntegration,
                       uniform bool singleShade,
                       uniform bool adaptiveSampling,
                       uniform float adaptiveScalar,
                       uniform float adaptiveMaxSamplingRate,
                       uniform float adaptiveBacktrack,
                       uniform float samplingRate,
                       const uniform vec3f &specular,
                       uniform float Ns,
                       void *uniform transferFunction,
                       const uniform box3f &clippingBox,
                       const uniform AffineSpace3f &xfm,
                       const uniform AffineSpace3f &rcp_xfm)
{
  Volume *uniform self = (Volume * uniform) _self;

  self->gradientShadingEnabled  = gradientShading;
  self->preIntegration          = preIntegration;
  self->singleShade             = singleShade;
  self->adaptiveSampling        = adaptiveSampling;
  self->adaptiveScalar          = adaptiveScalar;
  self->adaptiveMaxSamplingRate = adaptiveMaxSamplingRate;
  self->adaptiveBacktrack       = adaptiveBacktrack;
  self->samplingRate            = samplingRate;
  self->specular                = specular;
  self->ns                      = Ns;
  self->transferFunction        = (TransferFunction * uniform) transferFunction;
  self->volumeClippingBox       = clippingBox;
  self->xfm                     = xfm;
  self->rcp_xfm                 = rcp_xfm;
}
