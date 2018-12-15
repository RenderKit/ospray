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

#include "Material.ih"
#include "texture/TextureParam.ih"
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "math/spectrum.ih"

struct Metal
{
  PathTraceMaterial super;

  bool spectral;
  spectrum eta; // index of refraction
  spectrum k; // index of refraction, imaginary part
  vec3f etaRGB; // index of refraction
  vec3f kRGB; // index of refraction, imaginary part
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam map_roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying Metal_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Metal* uniform self = (const uniform Metal* uniform)super;
  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  Fresnel *uniform fresnel;
  if (self->spectral)
    fresnel = FresnelConductorSpectral_create(ctx, self->eta, self->k);
  else
    fresnel = FresnelConductorRGBUniform_create(ctx, self->etaRGB, self->kRGB);

  const float roughness = self->roughness
    * get1f(self->map_roughness, dg, 1.f);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, fresnel);
  else
    return MicrofacetConductor_create(ctx, frame, fresnel, roughness, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Metal_set(void* uniform _self
    , const float *uniform eta
    , const float *uniform k
    , const uniform vec3f& etaRGB
    , const uniform vec3f& kRGB
    , uniform float roughness
    , void* uniform map_roughness
    , const uniform affine2f &xform_roughness
    )
{
  Metal* uniform self = (Metal* uniform)_self;

  self->spectral = eta && k;
  if (self->spectral) {
    // actually could pass "uniform spectrum &", but may have different
    // alignment on C++ side, thus copy element-wise
    for (uniform int l = 0; l < SPECTRUM_SAMPLES; l++)  {
      self->eta[l] = eta[l];
      self->k[l] = k[l];
    }
  } else {
    self->etaRGB = etaRGB;
    self->kRGB = kRGB;
  }

  self->roughness = roughness;
  self->map_roughness =
    make_TextureParam((uniform Texture2D*)map_roughness,
        xform_roughness);
}

export void* uniform PathTracer_Metal_create()
{
  Metal* uniform self = uniform new Metal;
  PathTraceMaterial_Constructor(&self->super, Metal_getBSDF);
  uniform vec3f etaRGB = RGB_AL_ETA;
  uniform vec3f kRGB = RGB_AL_K;
  PathTracer_Metal_set(self, NULL, NULL, etaRGB, kRGB, 0.1f, NULL,
      make_AffineSpace2f_identity());
  return self;
}

