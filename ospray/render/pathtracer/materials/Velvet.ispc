// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "../bsdfs/Minneart.ih"
#include "../bsdfs/MultiBSDF.ih"
#include "../bsdfs/Velvety.ih"
#include "Material.ih"

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

const varying BSDF *varying Velvet_getBSDF(
    const uniform PathTraceMaterial *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Velvet *uniform self = (const uniform Velvet *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF *uniform bsdf = MultiBSDF_create(ctx);

  MultiBSDF_add(bsdf,
      Minneart_create(
          ctx, shadingFrame, self->reflectance, self->backScattering),
      1.f,
      luminance(self->reflectance));
  MultiBSDF_add(bsdf,
      Velvety_create(ctx,
          shadingFrame,
          self->horizonScatteringColor,
          self->horizonScatteringFallOff),
      1.f,
      luminance(self->horizonScatteringColor));

  return bsdf;
}

export void *uniform PathTracer_Velvet_create()
{
  uniform Velvet *uniform self = uniform new uniform Velvet;
  return self;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform PathTracer_Velvet_set(void *uniform _self,
    const uniform vec3f &reflectance,
    const uniform vec3f &horizonScatteringColor,
    uniform float horizonScatteringFallOff,
    uniform float backScattering)
{
  uniform Velvet *uniform self = (uniform Velvet * uniform) _self;
  self->reflectance = reflectance;
  self->horizonScatteringColor = horizonScatteringColor;
  self->horizonScatteringFallOff = horizonScatteringFallOff;
  self->backScattering = backScattering;
  PathTraceMaterial_Constructor(&self->super, Velvet_getBSDF);
}
