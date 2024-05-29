// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Principled.ih"

#include "Medium.ih"
#include "common/Ray.ih"
#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/Dielectric.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/LambertTransmission.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MicrofacetDielectric.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
#include "render/bsdfs/MicrofacetSheenLayer.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/OrenNayar.ih"
#include "render/bsdfs/PassthroughLayer.ih"
#include "render/bsdfs/ThinDielectric.ih"
#include "render/bsdfs/ThinMicrofacetDielectric.ih"
#include "render/bsdfs/Transmission.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "PrincipledShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

struct Principled_Diffuse_BSDF
{
  DEFINE_MULTIBSDF(2);
  OrenNayar lambert;
  BSDF lambertTransmission;
};

struct Principled_Plastic_BSDF
{
  MicrofacetDielectricLayer dielectricLayer;
  Principled_Diffuse_BSDF diffuse;
};

struct Principled_Glass_BSDF
{
  int bsdfId;
  MicrofacetDielectric dielectric;
  ThinMicrofacetDielectric thinDielectric;
  BSDF transmission;
};

struct Principled_Base_MultiBSDF
{
  DEFINE_MULTIBSDF(3);
  MicrofacetConductor metal;
  Principled_Glass_BSDF glass;
  Principled_Plastic_BSDF plastic;
};

struct Principled_ClearCoat_BSDF
{
  MicrofacetDielectricLayer dielectricLayer;
  Principled_Base_MultiBSDF base;
};

struct Principled_SheenLayer_BSDF
{
  MicrofacetSheenLayer sheenLayer;
  Principled_ClearCoat_BSDF clearCoat;
};

struct Principled_BSDF
{
  DEFINE_MULTIBSDF(2);
  BSDF transmission;
  Principled_SheenLayer_BSDF sheen;
};

SYCL_EXTERNAL const varying BSDF *uniform Principled_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &)
{
  const uniform Principled *uniform self =
      (const uniform Principled *uniform)super;

  // Allocate memory and initialize material BSDF
  varying Principled_BSDF *uniform bsdf = (varying Principled_BSDF * uniform)
      ShadingContext_alloc(ctx, sizeof(Principled_BSDF));
  varying MultiBSDF *uniform multiRoot = (varying MultiBSDF * uniform) bsdf;
  MultiBSDF_Constructor(multiRoot, 2);
  bsdf->super.bsdfType = BSDF_TYPE_PRINCIPLED;

  // create the default shading frame
  varying linear3f *uniform frame = LinearSpace3f_create(ctx,
      makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));
  const float rotation =
      clamp(self->rotation * get1f(self->rotationMap, dg, 1.f));
  const float theta = 2.f * (float)pi * rotation;
  if (theta > 0.f) {
    frame->vx = rotate(frame->vx, frame->vz, theta);
    frame->vy = cross(frame->vz, frame->vx);
  }

  float opacity = clamp(self->opacity * get1f(self->opacityMap, dg, 1.f));
  if (hasAlpha(self->baseColorMap)) {
    vec4f baseColorSample = get4f(self->baseColorMap, dg, make_vec4f(1.f));
    opacity *= baseColorSample.w;
  }

  if (self->opacity > EPS) {
    varying linear3f *uniform baseFrame = frame;
    if (valid(self->baseNormalMap)) {
      baseFrame = LinearSpace3f_create(ctx,
          makeShadingFrame(
              dg, self->baseNormalMap, self->baseNormalRot, self->baseNormal));
      if (theta > 0.f) {
        baseFrame->vx = rotate(baseFrame->vx, baseFrame->vz, theta);
        baseFrame->vy = cross(baseFrame->vz, baseFrame->vx);
      }
    }

    varying MultiBSDF *uniform multiBase =
        (varying MultiBSDF * uniform) & bsdf->sheen.clearCoat.base;
    MultiBSDF_Constructor(multiBase, 3);

    const vec3f baseColor =
        clamp(self->baseColor * get3f(self->baseColorMap, dg, make_vec3f(1.f))
            * make_vec3f(dg.color));
    const float specular =
        clamp(self->specular * get1f(self->specularMap, dg, 1.f));
    const float metallic =
        clamp(self->metallic * get1f(self->metallicMap, dg, 1.f));
    const float roughness =
        clamp(self->roughness * get1f(self->roughnessMap, dg, 1.f));
    const float anisotropy =
        clamp(self->anisotropy * get1f(self->anisotropyMap, dg, 1.f));
    const bool fromOutside =
        self->thin ? true : eq(currentMedium, self->outsideMedium);

    // dielectric base
    const float dielectric = (1.f - metallic);
    if (dielectric > EPS) {
      const float transmission =
          clamp(self->transmission * get1f(self->transmissionMap, dg, 1.f));

      float ior = self->ior * get1f(self->iorMap, dg, 1.f);
      if (ior < 1.f)
        ior = rcp(ior);
      float eta = fromOutside ? self->outsideMedium.ior * rcp(ior)
                              : ior * rcp(self->outsideMedium.ior);
      eta = clamp(eta, 1.f / 3.f, 3.f); // clamp to common range due to LUTs

      // plastic base
      const float plastic = dielectric * (1.f - transmission);
      if (plastic > EPS) {
        varying MultiBSDF *uniform multiDiffuse = (varying MultiBSDF * uniform)
            & bsdf->sheen.clearCoat.base.plastic.diffuse;
        MultiBSDF_Constructor(multiDiffuse, 2);

        varying Principled_Plastic_BSDF *uniform plasticBsdf =
            &bsdf->sheen.clearCoat.base.plastic;

        // diffuse
        const float diffuse =
            clamp(self->diffuse * get1f(self->diffuseMap, dg, 1.f));
        const vec3f diffuseColor = baseColor * diffuse;
        const float backlight = self->thin
            ? clamp(self->backlight * get1f(self->backlightMap, dg, 1.f),
                  0.f,
                  2.f)
            : 0.f;
        const float diffuseTransmission = backlight * 0.5f;
        const float diffuseReflection = 1.f - diffuseTransmission;

        if (diffuseReflection > EPS) {
          if (self->roughness < EPS)
            Lambert_Constructor(
                &plasticBsdf->diffuse.lambert.super, baseFrame, diffuseColor);
          else
            OrenNayar_Constructor(&plasticBsdf->diffuse.lambert,
                baseFrame,
                diffuseColor,
                roughness);

          MultiBSDF_add(multiDiffuse,
              0,
              &plasticBsdf->diffuse.lambert.super,
              diffuseReflection,
              diffuseReflection);
        }

        if (diffuseTransmission > EPS) {
          LambertTransmission_Constructor(
              &plasticBsdf->diffuse.lambertTransmission,
              baseFrame,
              diffuseColor);
          MultiBSDF_add(multiDiffuse,
              1,
              &plasticBsdf->diffuse.lambertTransmission,
              diffuseTransmission,
              diffuseTransmission);
        }

        // specular
        if (self->specular > EPS) {
          if (self->roughness < EPS) {
            DielectricLayer_Constructor((varying DielectricLayer * uniform)
                    & plasticBsdf->dielectricLayer,
                baseFrame,
                &plasticBsdf->diffuse.super,
                eta,
                make_vec3f(1.f),
                1.f,
                specular);
          } else {
            MicrofacetDielectricLayer_Constructor(&plasticBsdf->dielectricLayer,
                super->microfacetAlbedoTables,
                baseFrame,
                &plasticBsdf->diffuse.super,
                eta,
                make_vec3f(1.f),
                1.f,
                roughness,
                anisotropy,
                specular);
          }
        } else {
          PassthroughLayer_Constructor((varying PassthroughLayer * uniform)
                  & plasticBsdf->dielectricLayer,
              &plasticBsdf->diffuse.super);
        }

        MultiBSDF_add(multiBase,
            2,
            &plasticBsdf->dielectricLayer.super,
            plastic,
            plastic * max(diffuse, specular));
      }

      // glass base
      const float glass = dielectric * transmission * specular;
      if (glass > EPS) {
        varying Principled_Glass_BSDF *uniform glassBsdf =
            &bsdf->sheen.clearCoat.base.glass;

        if (abs(eta - 1.f) > EPS) {
          if (!self->thin) {
            if (self->roughness < EPS)
              Dielectric_Constructor(
                  (varying Dielectric * uniform) & glassBsdf->dielectric,
                  baseFrame,
                  eta);
            else
              MicrofacetDielectric_Constructor(&glassBsdf->dielectric,
                  super->microfacetAlbedoTables,
                  baseFrame,
                  eta,
                  roughness,
                  anisotropy);

            glassBsdf->bsdfId = 1;
            MultiBSDF_add(
                multiBase, 1, &glassBsdf->dielectric.super, glass, glass);
          } else {
            // thin
            const vec3f transmissionColor = clamp(self->transmissionColor
                * get3f(self->transmissionColorMap, dg, make_vec3f(1.f)));
            const float transmissionDepth = max(self->transmissionDepth
                    * get1f(self->transmissionDepthMap, dg, 1.f),
                EPS);
            const float thickness =
                max(self->thickness * get1f(self->thicknessMap, dg, 1.f), 0.f);
            const vec3f attenuation =
                logf(transmissionColor) / transmissionDepth * thickness;

            if (self->roughness < EPS)
              ThinDielectric_Constructor((varying ThinDielectric * uniform)
                      & glassBsdf->thinDielectric,
                  baseFrame,
                  eta,
                  attenuation);
            else
              ThinMicrofacetDielectric_Constructor(&glassBsdf->thinDielectric,
                  super->microfacetAlbedoTables,
                  baseFrame,
                  eta,
                  roughness,
                  anisotropy,
                  attenuation);

            glassBsdf->bsdfId = 2;
            MultiBSDF_add(
                multiBase, 1, &glassBsdf->thinDielectric.super, glass, glass);
          }
        } else {
          Transmission_Constructor(
              &glassBsdf->transmission, baseFrame, make_vec3f(1.f));
          glassBsdf->bsdfId = 0;
          MultiBSDF_add(multiBase, 1, &glassBsdf->transmission, glass, glass);
        }
      }
    }

    // conductor base
    const float conductor = metallic * specular;
    if (conductor > EPS) {
      const vec3f edgeColor = clamp(
          self->edgeColor * get3f(self->edgeColorMap, dg, make_vec3f(1.f)));

      Fresnel *uniform fresnel =
          FresnelConductorArtistic_create(ctx, baseColor, edgeColor);
      varying MicrofacetConductor *uniform metalBsdf =
          &bsdf->sheen.clearCoat.base.metal;
      if (self->roughness < EPS)
        Conductor_Constructor(
            (varying Conductor * uniform) metalBsdf, baseFrame, fresnel);
      else
        MicrofacetConductor_Constructor(metalBsdf,
            super->microfacetAlbedoTables,
            baseFrame,
            fresnel,
            roughness,
            anisotropy);

      MultiBSDF_add(multiBase, 0, &metalBsdf->super, conductor, conductor);
    }

    // coatings
    varying linear3f *uniform coatFrame = frame;

    // clear coat
    if (self->coat > EPS) {
      const float coat = clamp(self->coat * get1f(self->coatMap, dg, 1.f));
      float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
      if (coatIor < 1.f)
        coatIor = rcp(coatIor);
      float coatEta = fromOutside ? self->outsideMedium.ior * rcp(coatIor)
                                  : coatIor * rcp(self->outsideMedium.ior);
      coatEta =
          clamp(coatEta, 1.f / 3.f, 3.f); // clamp to common range due to LUTs

      const vec3f coatColor = clamp(
          self->coatColor * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
      const float coatThickness = max(
          self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f), 0.f);

      if (valid(self->coatNormalMap)) {
        coatFrame = LinearSpace3f_create(ctx,
            makeShadingFrame(dg,
                self->coatNormalMap,
                self->coatNormalRot,
                self->coatNormal));
      }

      if (self->coatRoughness < EPS || abs(coatEta - 1.f) < EPS) {
        DielectricLayer_Constructor((varying DielectricLayer * uniform)
                & bsdf->sheen.clearCoat.dielectricLayer,
            coatFrame,
            &bsdf->sheen.clearCoat.base.super,
            coatEta,
            coatColor,
            coatThickness,
            coat);
      } else {
        const float coatRoughness =
            clamp(self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f));
        MicrofacetDielectricLayer_Constructor(
            &bsdf->sheen.clearCoat.dielectricLayer,
            super->microfacetAlbedoTables,
            coatFrame,
            &bsdf->sheen.clearCoat.base.super,
            coatEta,
            coatColor,
            coatThickness,
            coatRoughness,
            0.f,
            coat);
      }
    } else {
      PassthroughLayer_Constructor((varying PassthroughLayer * uniform)
              & bsdf->sheen.clearCoat.dielectricLayer,
          &bsdf->sheen.clearCoat.base.super);
    }

    // sheen
    if (self->sheen > EPS) {
      const float sheen = clamp(self->sheen * get1f(self->sheenMap, dg, 1.f));
      vec3f sheenColor = clamp(
          self->sheenColor * get3f(self->sheenColorMap, dg, make_vec3f(1.f)));
      const float sheenTint =
          clamp(self->sheenTint * get1f(self->sheenTintMap, dg, 1.f));
      sheenColor = lerp(sheenTint, sheenColor, baseColor);
      const float sheenRoughness =
          clamp(self->sheenRoughness * get1f(self->sheenRoughnessMap, dg, 1.f));

      MicrofacetSheenLayer_Constructor(&bsdf->sheen.sheenLayer,
          super->microfacetAlbedoTables,
          coatFrame,
          &bsdf->sheen.clearCoat.dielectricLayer.super,
          sheenColor,
          sheenRoughness,
          sheen);
    } else
      PassthroughLayer_Constructor(
          (varying PassthroughLayer * uniform) & bsdf->sheen.sheenLayer,
          &bsdf->sheen.clearCoat.dielectricLayer.super);

    MultiBSDF_add(
        multiRoot, 1, &bsdf->sheen.sheenLayer.super, opacity, opacity);
  }

  // cut-out transparency
  if (1.f - self->opacity > EPS || valid(self->opacityMap)
      || hasAlpha(self->baseColorMap)) {
    const float transparency = 1.f - opacity;
    Transmission_Constructor(
        &bsdf->transmission, frame, make_vec3f(transparency));
    MultiBSDF_add(multiRoot, 0, &bsdf->transmission, 1.f, transparency);
  }

  return &bsdf->super;
}

SYCL_EXTERNAL vec3f Principled_getTransparency(
    const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &)
{
  const uniform Principled *uniform self =
      (const uniform Principled *uniform)super;
  vec3f T = make_vec3f(0.f);

  float opacity = clamp(self->opacity * get1f(self->opacityMap, dg, 1.f));
  if (hasAlpha(self->baseColorMap)) {
    vec4f baseColorSample = get4f(self->baseColorMap, dg, make_vec4f(1.f));
    opacity *= baseColorSample.w;
  }

  const float transparency = 1.f - opacity;
  const float transmission =
      clamp(self->transmission * get1f(self->transmissionMap, dg, 1.f));

  // early exit
  if (transparency <= EPS && transmission <= EPS)
    return T;

  // cut-out opacity
  if (self->opacity > EPS) {
    // glass base
    const float specular =
        clamp(self->specular * get1f(self->specularMap, dg, 1.f));
    const float metallic =
        clamp(self->metallic * get1f(self->metallicMap, dg, 1.f));
    const float dielectric = (1.f - metallic);
    const float glass = dielectric * transmission * specular;

    if (glass > EPS) {
      const bool fromOutside =
          self->thin ? true : eq(currentMedium, self->outsideMedium);
      float ior = self->ior * get1f(self->iorMap, dg, 1.f);
      if (ior < 1.f)
        ior = rcp(ior);
      float eta = fromOutside ? self->outsideMedium.ior * rcp(ior)
                              : ior * rcp(self->outsideMedium.ior);
      eta = clamp(eta, 1.f / 3.f, 3.f); // clamp to common range due to LUTs
      const float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.f);

      if (abs(eta - 1.f) > EPS) {
        if (!self->thin) {
          // solid
          // use microfacet if textured due to different
          // transparent shadow behavior
          if (self->roughness < EPS)
            T = Dielectric_getTransparency(cosThetaO, eta);
        } else {
          // thin
          const vec3f transmissionColor = clamp(self->transmissionColor
              * get3f(self->transmissionColorMap, dg, make_vec3f(1.f)));
          const float transmissionDepth = max(self->transmissionDepth
                  * get1f(self->transmissionDepthMap, dg, 1.f),
              EPS);
          const float thickness =
              max(self->thickness * get1f(self->thicknessMap, dg, 1.f), 0.f);
          const vec3f attenuation =
              logf(transmissionColor) / transmissionDepth * thickness;

          if (self->roughness < EPS) {
            T = ThinDielectric_getTransparency(cosThetaO, eta, attenuation);
          } else {
            const float roughness =
                clamp(self->roughness * get1f(self->roughnessMap, dg, 1.f));
            T = ThinMicrofacetDielectric_getTransparency(
                super->microfacetAlbedoTables,
                cosThetaO,
                eta,
                roughness,
                attenuation);
          }
        }
      } else {
        T = make_vec3f(1.f);
      }

      if (reduce_max(T) > 0.f) {
        // clear coat
        if (self->coat > EPS) {
          const float coat = clamp(self->coat * get1f(self->coatMap, dg, 1.f));
          float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
          if (coatIor < 1.f)
            coatIor = rcp(coatIor);
          float coatEta = fromOutside ? self->outsideMedium.ior * rcp(coatIor)
                                      : coatIor * rcp(self->outsideMedium.ior);
          coatEta = clamp(
              coatEta, 1.f / 3.f, 3.f); // clamp to common range due to LUTs

          if (abs(coatEta - 1.f) > EPS) {
            const float coatEta = fromOutside
                ? self->outsideMedium.ior * rcp(coatIor)
                : coatIor * rcp(self->outsideMedium.ior);
            const vec3f coatColor = clamp(self->coatColor
                * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
            const float coatThickness = max(
                self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f),
                0.f);

            if (self->coatRoughness < EPS) {
              T = T
                  * DielectricLayer_getTransparency(
                      cosThetaO, coatEta, coatColor, coatThickness, coat);
            } else {
              const float coatRoughness = clamp(
                  self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f));
              T = T
                  * MicrofacetDielectricLayer_getTransparency(
                      super->microfacetAlbedoTables,
                      cosThetaO,
                      coatEta,
                      coatColor,
                      coatThickness,
                      coatRoughness,
                      0.f,
                      coat);
            }
          }
        }

        // sheen
        if (self->sheen > EPS) {
          const float sheen =
              clamp(self->sheen * get1f(self->sheenMap, dg, 1.f));
          const float sheenRoughness = clamp(
              self->sheenRoughness * get1f(self->sheenRoughnessMap, dg, 1.f));

          T = T
              * MicrofacetSheenLayer_getTransparency(
                  super->microfacetAlbedoTables,
                  cosThetaO,
                  sheenRoughness,
                  sheen);
        }

        T = T * glass;
      }
    }
  }

  // cut-out transparency
  T = T * opacity + transparency;

  return T;
}

SYCL_EXTERNAL void Principled_selectNextMedium(
    const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{
  const uniform Principled *uniform self =
      (const uniform Principled *uniform)super;

  if (self->thin || self->transmission <= EPS)
    return;

  if (eq(currentMedium, self->outsideMedium)) {
    float ior = self->ior * get1f(self->iorMap, dg, 1.f);
    if (ior < 1.f)
      ior = rcp(ior);
    const vec3f transmissionColor = clamp(self->transmissionColor
        * get3f(self->transmissionColorMap, dg, make_vec3f(1.f)));
    const float transmissionDepth = max(
        self->transmissionDepth * get1f(self->transmissionDepthMap, dg, 1.f),
        EPS);

    currentMedium.ior = ior;
    currentMedium.attenuation = logf(transmissionColor) / transmissionDepth;
  } else {
    currentMedium = self->outsideMedium;
  }
}

// Lambert BSDF

inline BSDF_EvalRes LambertBSDF_eval(
    const varying OrenNayar *uniform self, const vec3f &wo, const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_OREN_NAYAR)
    return OrenNayar_eval(&self->super, wo, wi);
  else
    return Lambert_eval(&self->super, wo, wi);
}

inline BSDF_SampleRes LambertBSDF_sample(const varying OrenNayar *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_OREN_NAYAR)
    return OrenNayar_sample(&self->super, wo, s, ss);
  else
    return Lambert_sample(&self->super, wo, s, ss);
}

// Diffuse BSDF

__noinline BSDF_EvalRes DiffuseBSDF_eval(
    const varying Principled_Diffuse_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, LambertBSDF_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->lambertTransmission, LambertTransmission_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

__noinline BSDF_SampleRes DiffuseBSDF_sample(
    const varying Principled_Diffuse_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->lambert, LambertBSDF_sample);
  MULTIBSDF_SAMPLE_CHILD(
      1, &self->lambertTransmission, LambertTransmission_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, LambertBSDF_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->lambertTransmission, LambertTransmission_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

// Plastic DielectricLayer BSDF

inline BSDF_EvalRes Plastic_DielectricLayerBSDF_eval(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  DIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->diffuse.super.scatteringType,
      &self->diffuse,
      DiffuseBSDF_eval);
  return DIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes Plastic_DielectricLayerBSDF_sample(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  DIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->diffuse.super.scatteringType,
      &self->diffuse,
      DiffuseBSDF_sample);
  return DIELECTRICLAYER_SAMPLE_GET();
}

// Plastic MicrofacetDielectricLayer BSDF

inline BSDF_EvalRes Plastic_MicrofacetDielectricLayerBSDF_eval(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  MICROFACETDIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->diffuse.super.scatteringType,
      &self->diffuse,
      DiffuseBSDF_eval);
  return MICROFACETDIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes Plastic_MicrofacetDielectricLayerBSDF_sample(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  MICROFACETDIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->diffuse.super.scatteringType,
      &self->diffuse,
      DiffuseBSDF_eval,
      DiffuseBSDF_sample);
  return MICROFACETDIELECTRICLAYER_SAMPLE_GET();
}

// Plastic BSDF

__noinline BSDF_EvalRes PlasticBSDF_eval(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  // Skip dielectric layer if no specular
  if (self->dielectricLayer.weight < EPS)
    return DiffuseBSDF_eval(&self->diffuse, wo, wi);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return Plastic_MicrofacetDielectricLayerBSDF_eval(self, wo, wi);
  else
    return Plastic_DielectricLayerBSDF_eval(self, wo, wi);
}

inline BSDF_SampleRes PlasticBSDF_sample(
    const varying Principled_Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  // Skip dielectric layer if no specular
  if (self->dielectricLayer.weight < EPS)
    return DiffuseBSDF_sample(&self->diffuse, wo, s, ss);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return Plastic_MicrofacetDielectricLayerBSDF_sample(self, wo, s, ss);
  else
    return Plastic_DielectricLayerBSDF_sample(self, wo, s, ss);
}

// Glass Dielectric BSDF

inline BSDF_EvalRes Glass_DielectricBSDF_eval(
    const varying MicrofacetDielectric *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC)
    return MicrofacetDielectric_eval(&self->super, wo, wi);
  else
    return Dielectric_eval(&self->super, wo, wi);
}

inline BSDF_SampleRes Glass_DielectricBSDF_sample(
    const varying MicrofacetDielectric *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC)
    return MicrofacetDielectric_sample(&self->super, wo, s, ss);
  else
    return Dielectric_sample(&self->super, wo, s, ss);
}

// Glass ThinDielectric BSDF

inline BSDF_EvalRes Glass_ThinDielectricBSDF_eval(
    const varying ThinMicrofacetDielectric *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_THIN_MICROFACET_DIELECTRIC)
    return ThinMicrofacetDielectric_eval(&self->super, wo, wi);
  else
    return ThinDielectric_eval(&self->super, wo, wi);
}

inline BSDF_SampleRes Glass_ThinDielectricBSDF_sample(
    const varying ThinMicrofacetDielectric *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_THIN_MICROFACET_DIELECTRIC)
    return ThinMicrofacetDielectric_sample(&self->super, wo, s, ss);
  else
    return ThinDielectric_sample(&self->super, wo, s, ss);
}

// Glass BSDF

__noinline BSDF_EvalRes GlassBSDF_eval(
    const varying Principled_Glass_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  if (self->bsdfId == 1)
    return Glass_DielectricBSDF_eval(&self->dielectric, wo, wi);
  else if (self->bsdfId == 2)
    return Glass_ThinDielectricBSDF_eval(&self->thinDielectric, wo, wi);
  else
    return make_BSDF_EvalRes_zero();
}

inline BSDF_SampleRes GlassBSDF_sample(
    const varying Principled_Glass_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->bsdfId == 1)
    return Glass_DielectricBSDF_sample(&self->dielectric, wo, s, ss);
  else if (self->bsdfId == 2)
    return Glass_ThinDielectricBSDF_sample(&self->thinDielectric, wo, s, ss);
  else
    return Transmission_sample(&self->transmission, wo, s, ss);
}

// Metal BSDF

__noinline BSDF_EvalRes MetalBSDF_eval(
    const varying MicrofacetConductor *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
    return MicrofacetConductor_eval(self, wo, wi);
  else
    return make_BSDF_EvalRes_zero();
}

inline BSDF_SampleRes MetalBSDF_sample(
    const varying MicrofacetConductor *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
    return MicrofacetConductor_sample(self, wo, s, ss);
  else
    return Conductor_sample(&self->super, wo, s, ss);
}

// Base BSDF

__noinline BSDF_EvalRes BaseBSDF_eval(
    const varying Principled_Base_MultiBSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, &self->metal, MetalBSDF_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->glass, GlassBSDF_eval);
  MULTIBSDF_EVAL_CHILD(2, &self->plastic, PlasticBSDF_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

__noinline BSDF_SampleRes BaseBSDF_sample(
    const varying Principled_Base_MultiBSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->metal, MetalBSDF_sample);
  MULTIBSDF_SAMPLE_CHILD(1, &self->glass, GlassBSDF_sample);
  MULTIBSDF_SAMPLE_CHILD(2, &self->plastic, PlasticBSDF_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, &self->metal, MetalBSDF_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->glass, GlassBSDF_eval);
  MULTIBSDF_EVAL_CHILD(2, &self->plastic, PlasticBSDF_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

// ClearCoat DielectricLayer BSDF

inline BSDF_EvalRes ClearCoat_DielectricLayerBSDF_eval(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  DIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->base.super.scatteringType,
      &self->base,
      BaseBSDF_eval);
  return DIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes ClearCoat_DielectricLayerBSDF_sample(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  DIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->base.super.scatteringType,
      &self->base,
      BaseBSDF_sample);
  return DIELECTRICLAYER_SAMPLE_GET();
}

// ClearCoat MicrofacetDielectricLayer BSDF

inline BSDF_EvalRes ClearCoat_MicrofacetDielectricLayerBSDF_eval(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  MICROFACETDIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->base.super.scatteringType,
      &self->base,
      BaseBSDF_eval);
  return MICROFACETDIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes ClearCoat_MicrofacetDielectricLayerBSDF_sample(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  MICROFACETDIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->base.super.scatteringType,
      &self->base,
      BaseBSDF_eval,
      BaseBSDF_sample);
  return MICROFACETDIELECTRICLAYER_SAMPLE_GET();
}

// ClearCoat BSDF

__noinline BSDF_EvalRes ClearCoatBSDF_eval(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  // Skip dielectric layer if no clear coat
  if (self->dielectricLayer.weight < EPS)
    return BaseBSDF_eval(&self->base, wo, wi);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return ClearCoat_MicrofacetDielectricLayerBSDF_eval(self, wo, wi);
  else
    return ClearCoat_DielectricLayerBSDF_eval(self, wo, wi);
}

__noinline BSDF_SampleRes ClearCoatBSDF_sample(
    const varying Principled_ClearCoat_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  // Skip dielectric layer if no clear coat
  if (self->dielectricLayer.weight < EPS)
    return BaseBSDF_sample(&self->base, wo, s, ss);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return ClearCoat_MicrofacetDielectricLayerBSDF_sample(self, wo, s, ss);
  else
    return ClearCoat_DielectricLayerBSDF_sample(self, wo, s, ss);
}

// SheenLayer BSDF

__noinline BSDF_EvalRes SheenLayerBSDF_eval(
    const varying Principled_SheenLayer_BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  // Skip sheen layer if does not exists
  if (self->sheenLayer.weight < EPS)
    return ClearCoatBSDF_eval(&self->clearCoat, wo, wi);

  MICROFACETSHEENLAYER_EVAL(
      self->sheenLayer, &self->clearCoat, ClearCoatBSDF_eval);
  return MICROFACETSHEENLAYER_EVAL_GET();
}

inline BSDF_SampleRes SheenLayerBSDF_sample(
    const varying Principled_SheenLayer_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  // Skip sheen layer if does not exists
  if (self->sheenLayer.weight < EPS)
    return ClearCoatBSDF_sample(&self->clearCoat, wo, s, ss);

  MICROFACETSHEENLAYER_SAMPLE(self->sheenLayer,
      &self->clearCoat,
      ClearCoatBSDF_eval,
      ClearCoatBSDF_sample);
  return MICROFACETSHEENLAYER_SAMPLE_GET();
}

// Principled BSDF

SYCL_EXTERNAL BSDF_EvalRes Principled_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying Principled_BSDF *uniform self =
      (const varying Principled_BSDF *uniform)super;

  MULTIBSDF_EVAL_BEGIN();
  // 0 - omitted, no evaluation needed for transmission BSDF
  MULTIBSDF_EVAL_CHILD(1, &self->sheen, SheenLayerBSDF_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

SYCL_EXTERNAL BSDF_SampleRes Principled_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying Principled_BSDF *uniform self =
      (const varying Principled_BSDF *uniform)super;

  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->transmission, Transmission_sample);
  MULTIBSDF_SAMPLE_CHILD(1, &self->sheen, SheenLayerBSDF_sample);
  MULTIBSDF_SAMPLE_EVAL();
  // 0 - omitted, no evaluation needed for transmission BSDF
  MULTIBSDF_EVAL_CHILD(1, &self->sheen, SheenLayerBSDF_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

SYCL_EXTERNAL vec3f Principled_getEmission(const Material *uniform _self,
    const varying DifferentialGeometry &dg,
    const uniform FeatureFlagsHandler &)
{
  const Principled *uniform self = (Principled * uniform) _self;

  return self->emission * get3f(self->emissionMap, dg, make_vec3f(1.f));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Principled_getBSDF_addr()
{
  return (void *uniform)Principled_getBSDF;
}

export void *uniform Principled_getTransparency_addr()
{
  return (void *uniform)Principled_getTransparency;
}

export void *uniform Principled_selectNextMedium_addr()
{
  return (void *uniform)Principled_selectNextMedium;
}

export void *uniform Principled_getEmission_addr()
{
  return (void *uniform)Principled_getEmission;
}

OSPRAY_END_ISPC_NAMESPACE
