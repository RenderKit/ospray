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
  TextureParam baseColorMap;
  
  float metallic;
  TextureParam metallicMap;
  
  float specular; // dielectric specular reflectivity in [0, 1]; specular = sqr((ior-1)/(ior+1))/0.08
  TextureParam specularMap;
  
  float transmission;
  TextureParam transmissionMap;
  
  vec3f edgeColor; // metallic reflectivity at grazing angle (90 deg) / edge tint
  TextureParam edgeColorMap;
  
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam roughnessMap;

  TextureParam normalMap;
  linear2f normalRot;
  float normalScale;
  
  float coat; // dielectric clear coat reflectivity in [0, 1]; coat = sqr((ior-1)/(ior+1))/0.08
  TextureParam coatMap;
  
  vec3f coatColor;
  TextureParam coatColorMap;
  
  float coatThickness;
  TextureParam coatThicknessMap;
  
  float coatRoughness;
  TextureParam coatRoughnessMap;

  TextureParam coatNormalMap;
  linear2f coatNormalRot;
  float coatNormalScale;
  
  Medium mediumInside;
  Medium mediumOutside;
};

#define EPS 1e-5f

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying Principled_getBSDF(const uniform PathTraceMaterial* uniform super,
                                               uniform ShadingContext* uniform ctx,
                                               const DifferentialGeometry& dg,
                                               const Ray& ray,
                                               const Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)super;
  
  varying BSDF* uniform baseBsdf = MultiBSDF_create(ctx);
  varying linear3f* uniform frame =
    LinearSpace3f_create(ctx, makeShadingFrame(dg, self->normalMap, self->normalRot, self->normalScale));

  const vec3f baseColor = clamp(self->baseColor * get3f(self->baseColorMap, dg.st, make_vec3f(1.f)) * make_vec3f(dg.color));
  const float metallic = clamp(self->metallic * get1f(self->metallicMap, dg.st, 1.f));
  const float roughness = max(self->roughness * get1f(self->roughnessMap, dg.st, 1.f), 0.f);
  const float transmission = clamp(clamp(self->transmission * get1f(self->transmissionMap, dg.st, 1.f)));
  const float coat = max(self->coat * get1f(self->coatMap, dg.st, 1.f), 0.f);

  // plastic (diffuse+specular) base
  const float plastic = (1.f - metallic) * (1.f - transmission);
  if (plastic > EPS) {
    // diffuse
    varying BSDF* varying plasticBsdf;
    if (roughness < EPS)
      plasticBsdf = Lambert_create(ctx, frame, baseColor);
    else
      plasticBsdf = OrenNayar_create(ctx, frame, baseColor, roughness);
    
    // specular
    const float specular = max(self->specular * get1f(self->specularMap, dg.st, 1.f), 0.f);
    if (specular > EPS) {
      const float specularIor = (2.f / (1.f - sqrt(0.08f * specular))) - 1.f;
      
      if (roughness < EPS)
        plasticBsdf = DielectricLayer_create(ctx, frame, plasticBsdf, rcp(specularIor), make_vec3f(1.f), 1.f);
      else
        plasticBsdf = MicrofacetDielectricLayer_create(ctx, frame, plasticBsdf, rcp(specularIor), make_vec3f(1.f), 1.f, roughness);
    }

    MultiBSDF_add(baseBsdf, plasticBsdf, plastic, plastic);
  }
    
  // conductor base
  const float conductor = metallic * (1.f - transmission);
  if (conductor > EPS) {
    const vec3f edgeColor = clamp(self->edgeColor * get3f(self->edgeColorMap, dg.st, make_vec3f(1.f)));
    
    Fresnel* uniform fresnel = FresnelConductorArtistic_create(ctx, baseColor, edgeColor);
    varying BSDF* varying conductorBsdf;
    if (roughness < EPS)
      conductorBsdf = Conductor_create(ctx, frame, fresnel);
    else
      conductorBsdf = MicrofacetConductor_create(ctx, frame, fresnel, roughness);

    MultiBSDF_add(baseBsdf, conductorBsdf, conductor, conductor);
  }

  // transparent dielectric base
  if (transmission > EPS) {
    float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.ior*rcp(self->mediumInside.ior)
                                                       : self->mediumInside.ior*rcp(self->mediumOutside.ior);

    varying BSDF* varying dielectricBsdf;
    if (roughness < EPS)
      dielectricBsdf = Dielectric_create(ctx, frame, eta);
    else
      dielectricBsdf = MicrofacetDielectric_create(ctx, frame, eta, roughness);

    MultiBSDF_add(baseBsdf, dielectricBsdf, transmission, transmission);
  }

  varying BSDF* varying bsdf = baseBsdf;
  
  // clear coat
  if (coat > EPS) {
    const float coatIor = (2.f / (1.f - sqrt(0.08f * coat))) - 1.f;
    const vec3f coatColor = clamp(self->coatColor * get3f(self->coatColorMap, dg.st, make_vec3f(1.f)));
    const float coatThickness = max(self->coatThickness * get1f(self->coatThicknessMap, dg.st, 1.f), 0.f);
    const float coatRoughness = max(self->coatRoughness * get1f(self->coatRoughnessMap, dg.st, 1.f), 0.f);
    varying linear3f* uniform coatFrame =
      LinearSpace3f_create(ctx, makeShadingFrame(dg, self->coatNormalMap, self->coatNormalRot, self->coatNormalScale));
    
    if (coatRoughness < EPS)
      bsdf = DielectricLayer_create(ctx, coatFrame, bsdf, rcp(coatIor), coatColor, coatThickness);
    else
      bsdf = MicrofacetDielectricLayer_create(ctx, coatFrame, bsdf, rcp(coatIor), coatColor, coatThickness, coatRoughness);
  }

  return bsdf;
}

vec3f Principled_getTransparency(const uniform PathTraceMaterial* uniform material,
                                 const DifferentialGeometry& dg,
                                 const Ray& ray,
                                 const Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)material;

  const float transmission = self->transmission * get1f(self->transmissionMap, dg.st, 1.f);
  if (transmission < EPS)
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
    const uniform vec3f& baseColor, void* uniform baseColorMap, const uniform affine2f& baseColorXform,
    uniform float metallic, void* uniform metallicMap, const uniform affine2f& metallicXform,
    uniform float specular, void* uniform specularMap, const uniform affine2f& specularXform,
    const uniform vec3f& edgeColor, void* uniform edgeColorMap, const uniform affine2f& edgeColorXform,
    uniform float transmission, void* uniform transmissionMap, const uniform affine2f& transmissionXform,
    uniform float roughness, void* uniform roughnessMap, const uniform affine2f& roughnessXform,
    void* uniform normalMap, const uniform affine2f& normalXform, const uniform linear2f& normalRot, uniform float normalScale,
    uniform float coat, void* uniform coatMap, const uniform affine2f& coatXform,
    const uniform vec3f& coatColor, void* uniform coatColorMap, const uniform affine2f& coatColorXform,
    uniform float coatThickness, void* uniform coatThicknessMap, const uniform affine2f& coatThicknessXform,
    uniform float coatRoughness, void* uniform coatRoughnessMap, const uniform affine2f& coatRoughnessXform,
    void* uniform coatNormalMap, const uniform affine2f& coatNormalXform, const uniform linear2f& coatNormalRot, uniform float coatNormalScale,
    uniform float ior,
    const uniform vec3f& transmissionColor,
    uniform float transmissionDepth,
    uniform float iorOutside,
    const uniform vec3f& transmissionColorOutside,
    uniform float transmissionDepthOutside)
{
  Principled* uniform self = (Principled* uniform)_self;

  self->baseColor = baseColor;
  self->baseColorMap = make_TextureParam((uniform Texture2D*)baseColorMap, baseColorXform);
  
  self->metallic = metallic;
  self->metallicMap = make_TextureParam((uniform Texture2D*)metallicMap, metallicXform);
  
  self->specular = specular;
  self->specularMap = make_TextureParam((uniform Texture2D*)specularMap, specularXform);
  
  self->edgeColor = edgeColor;
  self->edgeColorMap = make_TextureParam((uniform Texture2D*)edgeColorMap, edgeColorXform);
  
  self->transmission = transmission;
  self->transmissionMap = make_TextureParam((uniform Texture2D*)transmissionMap, transmissionXform);

  self->roughness = roughness;
  self->roughnessMap = make_TextureParam((uniform Texture2D*)roughnessMap, roughnessXform);

  self->normalMap = make_TextureParam((uniform Texture2D*)normalMap, normalXform);
  self->normalRot = normalRot;
  self->normalScale = normalScale;
  
  self->coat = coat;
  self->coatMap = make_TextureParam((uniform Texture2D*)coatMap, coatXform);
  
  self->coatColor = coatColor;
  self->coatColorMap = make_TextureParam((uniform Texture2D*)coatColorMap, coatColorXform);
  
  self->coatThickness = coatThickness;
  self->coatThicknessMap = make_TextureParam((uniform Texture2D*)coatThicknessMap, coatThicknessXform);
  
  self->coatRoughness = coatRoughness;
  self->coatRoughnessMap = make_TextureParam((uniform Texture2D*)coatRoughnessMap, coatRoughnessXform);

  self->coatNormalMap = make_TextureParam((uniform Texture2D*)coatNormalMap, coatNormalXform);
  self->coatNormalRot = coatNormalRot;
  self->coatNormalScale = coatNormalScale;
  
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
    0.f, NULL, xform,
    0.f, NULL, xform,
    make_vec3f(1.f), NULL, xform,
    0.f, NULL, xform,
    0.f, NULL, xform,
    NULL, xform, make_LinearSpace2f_identity(), 1.f,
    0.f, NULL, xform,
    make_vec3f(1.f), NULL, xform,
    1.f, NULL, xform,
    0.f, NULL, xform,
    NULL, xform, make_LinearSpace2f_identity(), 1.f,
    1.5f,
    make_vec3f(1.f),
    1.f,
    1.f,
    make_vec3f(1.f),
    1.f);

  return self;
}
