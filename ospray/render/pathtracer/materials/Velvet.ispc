// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use self file except in compliance with the License.         //
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

#include "Material.ih"
#include "../bsdfs/MultiBSDF.ih"
#include "../bsdfs/Minneart.ih"
#include "../bsdfs/Velvety.ih"

struct Velvet
{
  PathTraceMaterial super;

  /*! Diffuse reflectance of the surface. The range is from 0
   *  (black) to 1 (white). */
  vec3f reflectance;

  /*! Amount of back scattering. The range is from 0 (no back
   *  scattering) to inf (maximum back scattering). */
  float backScattering;

  /*! Color of horizon scattering. */
  vec3f horizonScatteringColor;

  /*! Fall-off of horizon scattering. */
  float horizonScatteringFallOff;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Velvet_getBSDF(const uniform PathTraceMaterial* uniform super,
                                           uniform ShadingContext* uniform ctx,
                                           const DifferentialGeometry& dg,
                                           const Ray& ray,
                                           const Medium& currentMedium)
{
  const uniform Velvet* uniform self = (const uniform Velvet* uniform)super;

  varying LinearSpace3f* uniform shadingFrame = LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);

  MultiBSDF_add(bsdf, Minneart_create(ctx, shadingFrame, self->reflectance, self->backScattering), luminance(self->reflectance));
  MultiBSDF_add(bsdf, Velvety_create(ctx, shadingFrame, self->horizonScatteringColor, self->horizonScatteringFallOff), luminance(self->horizonScatteringColor));

  return bsdf;
}

void Velvet_Constructor(uniform Velvet* uniform self,
                        const uniform vec3f& reflectance,
                        const uniform vec3f& horizonScatteringColor,
                        uniform float horizonScatteringFallOff,
                        uniform float backScattering)
{
  PathTraceMaterial_Constructor(&self->super, Velvet_getBSDF, NULL, NULL, NULL);
  self->reflectance = reflectance;
  self->horizonScatteringColor = horizonScatteringColor;
  self->horizonScatteringFallOff = horizonScatteringFallOff;
  self->backScattering = backScattering;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Velvet_create(const uniform vec3f& reflectance,
                                              const uniform vec3f& horizonScatteringColor,
                                              uniform float horizonScatteringFallOff,
                                              uniform float backScattering)
{
  uniform Velvet* uniform self = uniform new uniform Velvet;
  Velvet_Constructor(self,reflectance,horizonScatteringColor,horizonScatteringFallOff,backScattering);
  return self;
}
