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
#include "../bsdfs/DielectricLayer.ih"
#include "../bsdfs/MicrofacetDielectricLayer.ih"
#include "../shaders/Flakes.ih"

struct CarPaint
{
  PathTraceMaterial super;

  vec3f baseColor;
  TextureParam baseColorMap;

  float roughness;
  TextureParam roughnessMap;

  float normal; // scale
  TextureParam normalMap;
  linear2f normalRot;

  float flakeScale;
  TextureParam flakeScaleMap;

  float flakeDensity;
  TextureParam flakeDensityMap;

  float flakeSpread;
  TextureParam flakeSpreadMap;

  float flakeJitter;
  TextureParam flakeJitterMap;

  float flakeRoughness;
  TextureParam flakeRoughnessMap;

  // dielectric clear coat reflectivity in [0, 1]
  float coat;
  TextureParam coatMap;

  // dielectric clear coat index of refraction in [1, 3]
  float coatIor;
  TextureParam coatIorMap;

  float coatThickness;
  TextureParam coatThicknessMap;

  float coatRoughness;
  TextureParam coatRoughnessMap;

  float coatNormal; // scale
  TextureParam coatNormalMap;
  linear2f coatNormalRot;

  vec3f coatColor;
  TextureParam coatColorMap;

  vec3f flipflopColor;
  TextureParam flipflopColorMap;

  float flipflopFalloff;
  TextureParam flipflopFalloffMap;
};

#define EPS 1e-5f

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying CarPaint_getBSDF(const uniform PathTraceMaterial* uniform super,
                                             uniform ShadingContext* uniform ctx,
                                             const DifferentialGeometry& dg,
                                             const Ray& ray,
                                             const Medium& currentMedium)
{
  const uniform CarPaint* uniform self = (const uniform CarPaint* uniform)super;
  varying BSDF* varying bsdf = NULL;

  varying linear3f* uniform frame =
    LinearSpace3f_create(ctx, makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));

  const float flakeDensity = clamp(self->flakeDensity * get1f(self->flakeDensityMap, dg, 1.f));
  int flakeMask = 0;
  varying linear3f* uniform flakeFrame = NULL;

  // metallic flakes in the clear coat layer
  if (flakeDensity > EPS)
  {
    const float flakeScale = max(self->flakeScale * get1f(self->flakeScaleMap, dg, 1.f), 0.f);
    const float flakeSpread = max(self->flakeSpread * get1f(self->flakeSpreadMap, dg, 1.f), 0.f);
    const float flakeJitter = clamp(self->flakeJitter * get1f(self->flakeJitterMap, dg, 1.f));

    Flakes flakes;
    flakes.scale = flakeScale;
    flakes.density = flakeDensity;
    flakes.spread = flakeSpread;
    flakes.jitter = flakeJitter;

    const vec3f flakeN = Flakes_eval(flakes, dg.P, flakeMask);
    if (flakeMask)
    {
      flakeFrame = LinearSpace3f_create(ctx, makeShadingFrame(dg, flakeN));

      // flakes are made of aluminum
      const uniform vec3f flakeEta = make_vec3f(1.69700277f, 0.879832864f, 0.5301736f);
      const uniform vec3f flakeK = make_vec3f(9.30200672f, 6.27604008f, 4.89433956f);
      Fresnel* uniform flakeFresnel = FresnelConductorRGBUniform_create(ctx, flakeEta, flakeK);

      const float flakeRoughness = max(self->flakeRoughness * get1f(self->flakeRoughnessMap, dg, 1.f), 0.f);
      if (flakeRoughness < EPS)
        bsdf = Conductor_create(ctx, flakeFrame, flakeFresnel);
      else
        bsdf = MicrofacetConductor_create(ctx, flakeFrame, flakeFresnel, flakeRoughness, 0.f);
    }
  }

  // base diffuse layer
  if (!flakeMask)
  {
    const vec3f baseColor = clamp(self->baseColor * get3f(self->baseColorMap, dg, make_vec3f(1.f)) * make_vec3f(dg.color));
    const float roughness = max(self->roughness * get1f(self->roughnessMap, dg, 1.f), 0.f);

    if (roughness < EPS)
      bsdf = Lambert_create(ctx, frame, baseColor);
    else
      bsdf = OrenNayar_create(ctx, frame, baseColor, roughness);
  }

  // clear coat layer
  const float coat = max(self->coat * get1f(self->coatMap, dg, 1.f), 0.f);
  if (coat > EPS)
  {
    float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
    if (coatIor < 1.f) coatIor = rcp(coatIor);
    coatIor = clamp(coatIor, 1.f, 3.f); // clamp to common range due to LUTs
    if (abs(coatIor-1.f) > EPS)
    {
      // compute the final coat color
      const vec3f coatColor = clamp(self->coatColor * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
      vec3f coatFinalColor = coatColor;
      if (flakeMask)
      {
        const float flipflopFalloff = clamp(self->flipflopFalloff * get1f(self->flipflopFalloffMap, dg, 1.f));
        if (flipflopFalloff < 1.f-EPS)
        {
          // pearlescent flakes
          const vec3f flipflopColor = clamp(self->flipflopColor * get3f(self->flipflopColorMap, dg, make_vec3f(1.f)));
          const float cosThetaO = max(-dot(ray.dir, flakeFrame->vz), 0.f);
          const float weight = pow(1.f - cosThetaO, rcp(1.f - flipflopFalloff)); // use Schlick for the blending weight
          coatFinalColor = lerp(weight, coatColor, flipflopColor);
        }
      }

      const float coatThickness = max(self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f), 0.f);
      const float coatRoughness = max(self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f), 0.f);
      varying linear3f* uniform coatFrame =
          LinearSpace3f_create(ctx, makeShadingFrame(dg, self->coatNormalMap, self->coatNormalRot, self->coatNormal));

      if (coatRoughness < EPS)
      {
        bsdf = DielectricLayer_create(ctx, coatFrame, bsdf, rcp(coatIor), coatFinalColor, coatThickness, 1.f);
      }
      else
      {
        bsdf = MicrofacetDielectricLayer_create(ctx, coatFrame, bsdf,
          rcp(coatIor), coatFinalColor, coatThickness, coatRoughness, 0.f, 1.f);
      }
    }
  }

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_CarPaint_set(void* uniform _self,
    const uniform vec3f& baseColor, void* uniform baseColorMap, const uniform affine2f& baseColorXform,
    uniform float roughness, void* uniform roughnessMap, const uniform affine2f& roughnessXform,
    uniform float normal, void* uniform normalMap, const uniform affine2f& normalXform, const uniform linear2f& normalRot,
    uniform float flakeScale, void* uniform flakeScaleMap, const uniform affine2f& flakeScaleXform,
    uniform float flakeDensity, void* uniform flakeDensityMap, const uniform affine2f& flakeDensityXform,
    uniform float flakeSpread, void* uniform flakeSpreadMap, const uniform affine2f& flakeSpreadXform,
    uniform float flakeJitter, void* uniform flakeJitterMap, const uniform affine2f& flakeJitterXform,
    uniform float flakeRoughness, void* uniform flakeRoughnessMap, const uniform affine2f& flakeRoughnessXform,
    uniform float coat, void* uniform coatMap, const uniform affine2f& coatXform,
    uniform float coatIor, void* uniform coatIorMap, const uniform affine2f& coatIorXform,
    const uniform vec3f& coatColor, void* uniform coatColorMap, const uniform affine2f& coatColorXform,
    uniform float coatThickness, void* uniform coatThicknessMap, const uniform affine2f& coatThicknessXform,
    uniform float coatRoughness, void* uniform coatRoughnessMap, const uniform affine2f& coatRoughnessXform,
    uniform float coatNormal, void* uniform coatNormalMap, const uniform affine2f& coatNormalXform, const uniform linear2f& coatNormalRot,
    const uniform vec3f& flipflopColor, void* uniform flipflopColorMap, const uniform affine2f& flipflopColorXform,
    uniform float flipflopFalloff, void* uniform flipflopFalloffMap, const uniform affine2f& flipflopFalloffXform)
{
  CarPaint* uniform self = (CarPaint* uniform)_self;

  self->baseColor = baseColor;
  self->baseColorMap = make_TextureParam((uniform Texture2D*)baseColorMap, baseColorXform);

  self->roughness = roughness;
  self->roughnessMap = make_TextureParam((uniform Texture2D*)roughnessMap, roughnessXform);

  self->normal = normal;
  self->normalMap = make_TextureParam((uniform Texture2D*)normalMap, normalXform);
  self->normalRot = normalRot;

  self->flakeScale = flakeScale;
  self->flakeScaleMap = make_TextureParam((uniform Texture2D*)flakeScaleMap, flakeScaleXform);

  self->flakeDensity = flakeDensity;
  self->flakeDensityMap = make_TextureParam((uniform Texture2D*)flakeDensityMap, flakeDensityXform);

  self->flakeSpread = flakeSpread;
  self->flakeSpreadMap = make_TextureParam((uniform Texture2D*)flakeSpreadMap, flakeSpreadXform);

  self->flakeJitter = flakeJitter;
  self->flakeJitterMap = make_TextureParam((uniform Texture2D*)flakeJitterMap, flakeJitterXform);

  self->flakeRoughness = flakeRoughness;
  self->flakeRoughnessMap = make_TextureParam((uniform Texture2D*)flakeRoughnessMap, flakeRoughnessXform);

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

  self->flipflopColor = flipflopColor;
  self->flipflopColorMap = make_TextureParam((uniform Texture2D*)flipflopColorMap, flipflopColorXform);

  self->flipflopFalloff = flipflopFalloff;
  self->flipflopFalloffMap = make_TextureParam((uniform Texture2D*)flipflopFalloffMap, flipflopFalloffXform);
}

export void* uniform PathTracer_CarPaint_create()
{
  CarPaint* uniform self = uniform new CarPaint;

  PathTraceMaterial_Constructor(&self->super, CarPaint_getBSDF);

  uniform affine2f xform = make_AffineSpace2f_identity();
  PathTracer_CarPaint_set(self,
    make_vec3f(0.8f), NULL, xform, // baseColor
    0.f, NULL, xform, // roughness
    1.f, NULL, xform, make_LinearSpace2f_identity(), // normal
    0.f, NULL, xform, // flakeScale
    0.f, NULL, xform, // flakeDensity
    0.f, NULL, xform, // flakeSpread
    0.f, NULL, xform, // flakeJitter
    0.f, NULL, xform, // flakeRoughness
    1.f, NULL, xform, // coat
    1.5f, NULL, xform, // coatIor
    make_vec3f(1.f), NULL, xform, // coatColor
    1.f, NULL, xform, // coatThickness
    0.f, NULL, xform, // coatRoughness
    1.f, NULL, xform, make_LinearSpace2f_identity(), // coatNormal
    make_vec3f(1.f), NULL, xform, // flipflopColor
    1.f, NULL, xform // flipflopFalloff
  );

  return self;
}
