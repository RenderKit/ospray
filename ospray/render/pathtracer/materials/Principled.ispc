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

  vec3f edgeColor; // metallic reflectivity at grazing angle (90 deg) / edge tint
  TextureParam edgeColorMap;
  
  float metallic;
  TextureParam metallicMap;
  
  float specular; // specular reflectivity in [0, 1]
  TextureParam specularMap;

  float ior;
  TextureParam iorMap; // currently this is ignored for transmission

  float transmission;
  TextureParam transmissionMap;

  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam roughnessMap;

  float normal; // scale
  TextureParam normalMap;
  linear2f normalRot;
  
  float coat; // dielectric clear coat reflectivity in [0, 1]
  TextureParam coatMap;

  float coatIor;
  TextureParam coatIorMap;
  
  vec3f coatColor;
  TextureParam coatColorMap;
  
  float coatThickness;
  TextureParam coatThicknessMap;
  
  float coatRoughness;
  TextureParam coatRoughnessMap;

  float coatNormal; // scale
  TextureParam coatNormalMap;
  linear2f coatNormalRot;
  
  Medium insideMedium;
  Medium outsideMedium;
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
    LinearSpace3f_create(ctx, makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));

  const vec3f baseColor = clamp(self->baseColor * get3f(self->baseColorMap, dg.st, make_vec3f(1.f)) * make_vec3f(dg.color));
  const float metallic = clamp(self->metallic * get1f(self->metallicMap, dg.st, 1.f));
  const float roughness = max(self->roughness * get1f(self->roughnessMap, dg.st, 1.f), 0.f);
  const float transmission = clamp(clamp(self->transmission * get1f(self->transmissionMap, dg.st, 1.f)));
  const float coat = max(self->coat * get1f(self->coatMap, dg.st, 1.f), 0.f);

  // dielectric base
  const float dielectric = (1.f - metallic);
  if (dielectric > EPS) {
    const float specular = clamp(self->specular * get1f(self->specularMap, dg.st, 1.f));

    // opaque dielectric base
    const float opaque = dielectric * (1.f - transmission);
    if (opaque > EPS) {
      // diffuse
      varying BSDF* varying opaqueBsdf;
      if (roughness < EPS)
        opaqueBsdf = Lambert_create(ctx, frame, baseColor);
      else
        opaqueBsdf = OrenNayar_create(ctx, frame, baseColor, roughness);
      
      // specular
      if (specular > EPS) {
        float ior = max(self->ior * get1f(self->iorMap, dg.st, 1.f), 0.f);
        if (ior < 1.f) ior = rcp(ior);

        if (abs(ior-1.f) > EPS) {
          if (roughness < EPS)
            opaqueBsdf = DielectricLayer_create(ctx, frame, opaqueBsdf, rcp(ior), make_vec3f(1.f), 1.f, specular);
          else
            opaqueBsdf = MicrofacetDielectricLayer_create(ctx, frame, opaqueBsdf, rcp(ior), make_vec3f(1.f), 1.f, roughness, specular);
        }
      }

      MultiBSDF_add(baseBsdf, opaqueBsdf, opaque, opaque);
    }

    // transparent dielectric base
    const float transparent = dielectric * transmission;
    if (transparent > EPS) {
      const float eta = eq(currentMedium, self->outsideMedium) ? self->outsideMedium.ior*rcp(self->insideMedium.ior)
                                                               : self->insideMedium.ior*rcp(self->outsideMedium.ior);

      varying BSDF* varying transparentBsdf;
      if (roughness < EPS)
        transparentBsdf = Dielectric_create(ctx, frame, eta, specular);
      else
        transparentBsdf = MicrofacetDielectric_create(ctx, frame, eta, roughness, specular);

      MultiBSDF_add(baseBsdf, transparentBsdf, transparent, transparent);
    }
  }
    
  // conductor base
  const float conductor = metallic;
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

  varying BSDF* varying bsdf = baseBsdf;
  
  // clear coat
  if (coat > EPS) {
    float coatIor = max(self->coatIor * get1f(self->coatIorMap, dg.st, 1.f), 0.f);
    if (coatIor < 1.f) coatIor = rcp(coatIor);
    const vec3f coatColor = clamp(self->coatColor * get3f(self->coatColorMap, dg.st, make_vec3f(1.f)));
    const float coatThickness = max(self->coatThickness * get1f(self->coatThicknessMap, dg.st, 1.f), 0.f);
    const float coatRoughness = max(self->coatRoughness * get1f(self->coatRoughnessMap, dg.st, 1.f), 0.f);
    varying linear3f* uniform coatFrame =
      LinearSpace3f_create(ctx, makeShadingFrame(dg, self->coatNormalMap, self->coatNormalRot, self->coatNormal));
    
    if (coatRoughness < EPS)
      bsdf = DielectricLayer_create(ctx, coatFrame, bsdf, rcp(coatIor), coatColor, coatThickness, coat);
    else
      bsdf = MicrofacetDielectricLayer_create(ctx, coatFrame, bsdf, rcp(coatIor), coatColor, coatThickness, coatRoughness, coat);
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

  float eta = eq(currentMedium, self->outsideMedium) ? self->outsideMedium.ior*rcp(self->insideMedium.ior)
                                                     : self->insideMedium.ior*rcp(self->outsideMedium.ior);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f((1.0f-fresnelDielectric(cosThetaO, eta)) * transmission);
}

void Principled_selectNextMedium(const uniform PathTraceMaterial* uniform super,
                                 const DifferentialGeometry& dg,
                                 Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)super;

  if (eq(currentMedium, self->outsideMedium))
    currentMedium = self->insideMedium;
  else
    currentMedium = self->outsideMedium;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Principled_set(void* uniform _self,
    const uniform vec3f& baseColor, void* uniform baseColorMap, const uniform affine2f& baseColorXform,
    const uniform vec3f& edgeColor, void* uniform edgeColorMap, const uniform affine2f& edgeColorXform,
    uniform float metallic, void* uniform metallicMap, const uniform affine2f& metallicXform,
    uniform float specular, void* uniform specularMap, const uniform affine2f& specularXform,
    uniform float ior, void* uniform iorMap, const uniform affine2f& iorXform,
    uniform float transmission, void* uniform transmissionMap, const uniform affine2f& transmissionXform,
    const uniform vec3f& transmissionColor,
    uniform float transmissionDepth,
    uniform float roughness, void* uniform roughnessMap, const uniform affine2f& roughnessXform,
    uniform float normal, void* uniform normalMap, const uniform affine2f& normalXform, const uniform linear2f& normalRot,
    uniform float coat, void* uniform coatMap, const uniform affine2f& coatXform,
    uniform float coatIor, void* uniform coatIorMap, const uniform affine2f& coatIorXform,
    const uniform vec3f& coatColor, void* uniform coatColorMap, const uniform affine2f& coatColorXform,
    uniform float coatThickness, void* uniform coatThicknessMap, const uniform affine2f& coatThicknessXform,
    uniform float coatRoughness, void* uniform coatRoughnessMap, const uniform affine2f& coatRoughnessXform,
    uniform float coatNormal, void* uniform coatNormalMap, const uniform affine2f& coatNormalXform, const uniform linear2f& coatNormalRot,
    uniform float outsideIor,
    const uniform vec3f& outsideTransmissionColor,
    uniform float outsideTransmissionDepth)
{
  Principled* uniform self = (Principled* uniform)_self;

  self->baseColor = baseColor;
  self->baseColorMap = make_TextureParam((uniform Texture2D*)baseColorMap, baseColorXform);

  self->edgeColor = edgeColor;
  self->edgeColorMap = make_TextureParam((uniform Texture2D*)edgeColorMap, edgeColorXform);
  
  self->metallic = metallic;
  self->metallicMap = make_TextureParam((uniform Texture2D*)metallicMap, metallicXform);
  
  self->specular = specular;
  self->specularMap = make_TextureParam((uniform Texture2D*)specularMap, specularXform);

  self->ior = ior;
  self->iorMap = make_TextureParam((uniform Texture2D*)iorMap, iorXform);
  
  self->transmission = transmission;
  self->transmissionMap = make_TextureParam((uniform Texture2D*)transmissionMap, transmissionXform);

  self->roughness = roughness;
  self->roughnessMap = make_TextureParam((uniform Texture2D*)roughnessMap, roughnessXform);

  self->normal = normal;
  self->normalMap = make_TextureParam((uniform Texture2D*)normalMap, normalXform);
  self->normalRot = normalRot;
  
  self->coat = coat;
  self->coatMap = make_TextureParam((uniform Texture2D*)coatMap, coatXform);

  self->coatIor = coatIor;
  self->coatIorMap = make_TextureParam((uniform Texture2D*)coatIorMap, coatIorXform);
  
  self->coatColor = coatColor;
  self->coatColorMap = make_TextureParam((uniform Texture2D*)coatColorMap, coatColorXform);
  
  self->coatThickness = coatThickness;
  self->coatThicknessMap = make_TextureParam((uniform Texture2D*)coatThicknessMap, coatThicknessXform);
  
  self->coatRoughness = coatRoughness;
  self->coatRoughnessMap = make_TextureParam((uniform Texture2D*)coatRoughnessMap, coatRoughnessXform);

  self->coatNormal = coatNormal;
  self->coatNormalMap = make_TextureParam((uniform Texture2D*)coatNormalMap, coatNormalXform);
  self->coatNormalRot = coatNormalRot;
  
  self->insideMedium.ior = ior;
  self->insideMedium.attenuation = logf(transmissionColor) / transmissionDepth;
  
  self->outsideMedium.ior = outsideIor;
  self->outsideMedium.attenuation = logf(outsideTransmissionColor) / outsideTransmissionDepth;
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
    make_vec3f(0.8f), NULL, xform, // baseColor
    make_vec3f(1.f), NULL, xform, // edgeColor
    0.f, NULL, xform, // metallic
    1.f, NULL, xform, // specular
    1.52f, NULL, xform, // ior
    0.f, NULL, xform, // transmission
    make_vec3f(1.f), // transmissionColor
    1.f, // transmissionDepth
    0.5f, NULL, xform, // roughness
    1.f, NULL, xform, make_LinearSpace2f_identity(), // normal
    0.f, NULL, xform, // coat
    1.5f, NULL, xform, // coatIor
    make_vec3f(1.f), NULL, xform, // coatColor
    1.f, NULL, xform, // coatThickness
    0.f, NULL, xform, // coatRoughness
    1.f, NULL, xform, make_LinearSpace2f_identity(), // coatNormal
    1.f, // outsideIor
    make_vec3f(1.f), // outsideTransmissionColor
    1.f // outsideTransmissionDepth
  );

  return self;
}
