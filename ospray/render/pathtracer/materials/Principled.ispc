// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "render/pathtracer/bsdfs/MultiBSDF.ih"
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/OrenNayar.ih"
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "../bsdfs/Dielectric.ih"
#include "../bsdfs/MicrofacetDielectric.ih"
#include "../bsdfs/DielectricLayer.ih"
#include "../bsdfs/MicrofacetDielectricLayer.ih"
#include "math/spectrum.ih"

struct Principled
{
  PathTraceMaterial super;

  vec3f baseColor;
  TextureParam map_baseColor;
  
  float metallic;
  TextureParam map_metallic;
  
  float specular; // dielectric specular reflectivity in [0, 1]; specular = sqr((ior-1)/(ior+1))/0.08
  TextureParam map_specular;
  
  float transmission;
  TextureParam map_transmission;
  
  vec3f edgeColor; // metallic reflectivity at grazing angle (90 deg) / edge tint
  TextureParam map_edgeColor;
  
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam map_roughness;
  
  float coat; // dielectric clear coat reflectivity in [0, 1]; coat = sqr((ior-1)/(ior+1))/0.08
  TextureParam map_coat;
  
  vec3f coatColor;
  TextureParam map_coatColor;
  
  float coatThickness;
  TextureParam map_coatThickness;
  
  float coatRoughness;
  TextureParam map_coatRoughness;
  
  Medium mediumInside;
  Medium mediumOutside;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Principled_getBSDF(const uniform PathTraceMaterial* uniform super,
                                               uniform ShadingContext* uniform ctx,
                                               const DifferentialGeometry& dg,
                                               const Ray& ray,
                                               const Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)super;
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);
  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f baseColor = self->baseColor * get3f(self->map_baseColor, dg.st, make_vec3f(1.f)) * make_vec3f(dg.color);
  const float metallic = self->metallic * get1f(self->map_metallic, dg.st, 1.f);
  const float roughness = self->roughness * get1f(self->map_roughness, dg.st, 1.f);
  const float transmission = self->transmission * get1f(self->map_transmission, dg.st, 1.f);
  const float coat = self->coat * get1f(self->map_coat, dg.st, 1.f);

  // opaque base: diffuse+specular
  const float diffuse = (1.f - metallic) * (1.f - transmission);
  if (diffuse > 1e-5f) {
    // diffuse
    varying BSDF* uniform diffuseBsdf;
    if (roughness < 1e-5f)
      diffuseBsdf = Lambert_create(ctx, frame, baseColor);
    else
      diffuseBsdf = OrenNayar_create(ctx, frame, baseColor, roughness);
    
    // specular
    const float specular = self->specular * get1f(self->map_specular, dg.st, 1.f);
    if (specular > 1e-5f) {
      const float specularIor = (2.f / (1.f - sqrt(0.08f * specular))) - 1.f;
      
      if (roughness < 1e-5f)
        diffuseBsdf = DielectricLayer_create(ctx, frame, diffuseBsdf, rcp(specularIor), make_vec3f(1.f), 1.f);
      else
        diffuseBsdf = MicrofacetDielectricLayer_create(ctx, frame, diffuseBsdf, rcp(specularIor), make_vec3f(1.f), 1.f, roughness);
    }

    MultiBSDF_add(bsdf, diffuseBsdf, diffuse, luminance(baseColor)*diffuse);
  }
    
  // opaque base: conductor
  const float conductor = metallic * (1.f - transmission);
  if (conductor > 1e-5f) {
    const vec3f edgeColor = self->edgeColor * get3f(self->map_edgeColor, dg.st, make_vec3f(1.f));
    
    Fresnel* uniform fresnel = FresnelConductorArtistic_create(ctx, baseColor, edgeColor);
    varying BSDF* uniform conductorBsdf;
    if (roughness < 1e-5f)
      conductorBsdf = Conductor_create(ctx, frame, fresnel);
    else
      conductorBsdf = MicrofacetConductor_create(ctx, frame, fresnel, roughness);

    MultiBSDF_add(bsdf, conductorBsdf, conductor, luminance(baseColor)*conductor);
  }

  // transparent base
  if (transmission > 1e-5f) {
    float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.ior*rcp(self->mediumInside.ior)
                                                       : self->mediumInside.ior*rcp(self->mediumOutside.ior);

    varying BSDF* uniform dielectricBsdf;
    if (roughness < 1e-5f)
      dielectricBsdf = Dielectric_create(ctx, frame, eta);
    else
      dielectricBsdf = MicrofacetDielectric_create(ctx, frame, eta, roughness);

    MultiBSDF_add(bsdf, dielectricBsdf, transmission, transmission);
  }
  
  // clear coat
  if (coat > 1e-5f) {
    const float coatIor = (2.f / (1.f - sqrt(0.08f * coat))) - 1.f;
    const vec3f coatColor = self->coatColor * get3f(self->map_coatColor, dg.st, make_vec3f(1.f));
    const float coatThickness = self->coatThickness * get1f(self->map_coatThickness, dg.st, 1.f);
    const float coatRoughness = self->coatRoughness * get1f(self->map_coatRoughness, dg.st, 1.f);
    
    if (coatRoughness < 1e-5f)
      bsdf = DielectricLayer_create(ctx, frame, bsdf, rcp(coatIor), coatColor, coatThickness);
    else
      bsdf = MicrofacetDielectricLayer_create(ctx, frame, bsdf, rcp(coatIor), coatColor, coatThickness, coatRoughness);
  }

  return bsdf;
}

vec3f Principled_getTransparency(const uniform PathTraceMaterial* uniform material,
                                 const DifferentialGeometry& dg,
                                 const Ray& ray,
                                 const Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)material;

  const float transmission = self->transmission * get1f(self->map_transmission, dg.st, 1.f);
  if (transmission < 1e-5f)
    return make_vec3f(0.f);

  float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.ior*rcp(self->mediumInside.ior)
                                                     : self->mediumInside.ior*rcp(self->mediumOutside.ior);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f((1.0f-fresnelDielectric(cosThetaO, eta)) * transmission);
}

void Principled_selectNextMedium(const uniform PathTraceMaterial* uniform super,
                                 Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)super;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Principled_set(void* uniform _self,
    const uniform vec3f& baseColor, void* uniform map_baseColor, const uniform affine2f& xform_baseColor,
    uniform float metallic, void* uniform map_metallic, const uniform affine2f& xform_metallic,
    uniform float specular, void* uniform map_specular, const uniform affine2f& xform_specular,
    const uniform vec3f& edgeColor, void* uniform map_edgeColor, const uniform affine2f& xform_edgeColor,
    uniform float transmission, void* uniform map_transmission, const uniform affine2f& xform_transmission,
    uniform float roughness, void* uniform map_roughness, const uniform affine2f& xform_roughness,
    uniform float coat, void* uniform map_coat, const uniform affine2f& xform_coat,
    const uniform vec3f& coatColor, void* uniform map_coatColor, const uniform affine2f& xform_coatColor,
    uniform float coatThickness, void* uniform map_coatThickness, const uniform affine2f& xform_coatThickness,
    uniform float coatRoughness, void* uniform map_coatRoughness, const uniform affine2f& xform_coatRoughness,
    uniform float ior,
    const uniform vec3f& transmissionColor,
    uniform float transmissionDepth,
    uniform float iorOutside,
    const uniform vec3f& transmissionColorOutside,
    uniform float transmissionDepthOutside)
{
  Principled* uniform self = (Principled* uniform)_self;

  self->baseColor = baseColor;
  self->map_baseColor = make_TextureParam((uniform Texture2D*)map_baseColor, xform_baseColor);
  
  self->metallic = metallic;
  self->map_metallic = make_TextureParam((uniform Texture2D*)map_metallic, xform_metallic);
  
  self->specular = specular;
  self->map_specular = make_TextureParam((uniform Texture2D*)map_specular, xform_specular);
  
  self->edgeColor = edgeColor;
  self->map_edgeColor = make_TextureParam((uniform Texture2D*)map_edgeColor, xform_edgeColor);
  
  self->transmission = transmission;
  self->map_transmission = make_TextureParam((uniform Texture2D*)map_transmission, xform_transmission);

  self->roughness = roughness;
  self->map_roughness = make_TextureParam((uniform Texture2D*)map_roughness, xform_roughness);
  
  self->coat = coat;
  self->map_coat = make_TextureParam((uniform Texture2D*)map_coat, xform_coat);
  
  self->coatColor = coatColor;
  self->map_coatColor = make_TextureParam((uniform Texture2D*)map_coatColor, xform_coatColor);
  
  self->coatThickness = coatThickness;
  self->map_coatThickness = make_TextureParam((uniform Texture2D*)map_coatThickness, xform_coatThickness);
  
  self->coatRoughness = coatRoughness;
  self->map_coatRoughness = make_TextureParam((uniform Texture2D*)map_coatRoughness, xform_coatRoughness);
  
  self->mediumInside.ior = ior;
  self->mediumInside.attenuation = logf(transmissionColor) / transmissionDepth;
  
  self->mediumOutside.ior = iorOutside;
  self->mediumOutside.attenuation = logf(transmissionColorOutside) / transmissionDepthOutside;
}

export void* uniform PathTracer_Principled_create()
{
  Principled* uniform self = uniform new Principled;

  PathTraceMaterial_Constructor(&self->super,
    Principled_getBSDF,
    Principled_getTransparency,
    Principled_selectNextMedium);

  uniform affine2f xform = make_AffineSpace2f_identity();
  PathTracer_Principled_set(self,
    make_vec3f(0.8f), NULL, xform,
    0.0f, NULL, xform,
    0.0f, NULL, xform,
    make_vec3f(1.f), NULL, xform,
    0.0f, NULL, xform,
    0.1f, NULL, xform,
    0.0f, NULL, xform,
    make_vec3f(1.f), NULL, xform,
    1.0f, NULL, xform,
    0.0f, NULL, xform,
    1.5f,
    make_vec3f(1.f),
    1.f,
    1.f,
    make_vec3f(1.f),
    1.f);

  return self;
}
