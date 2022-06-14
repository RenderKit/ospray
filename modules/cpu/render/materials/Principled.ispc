// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Medium.ih"
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
#include "render/bsdfs/ThinDielectric.ih"
#include "render/bsdfs/ThinMicrofacetDielectric.ih"
#include "render/bsdfs/Transmission.ih"
#include "texture/TextureParam.ih"

#include "PrincipledShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Principled_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Principled *uniform self =
      (const uniform Principled *uniform)super;
  varying BSDF *varying bsdf = NULL;

  // create the default shading frame
  varying linear3f *uniform frame = LinearSpace3f_create(ctx,
      makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));
  const float rotation =
      clamp(self->rotation * get1f(self->rotationMap, dg, 1.f));
  const float theta = 2.f * pi * rotation;
  if (theta > 0.f) {
    frame->vx = rotate(frame->vx, frame->vz, theta);
    frame->vy = cross(frame->vz, frame->vx);
  }

  float opacity = clamp(self->opacity * get1f(self->opacityMap, dg, 1.f));
  if (hasAlpha(self->baseColorMap)) {
    vec4f baseColorSample = get4f(self->baseColorMap, dg, make_vec4f(1.f));
    opacity *= baseColorSample.w;
  }

  if (opacity > EPS) {
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

    varying BSDF *uniform baseBsdf = MultiBSDF_create(ctx);

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
        varying BSDF *varying plasticBsdf = NULL;

        // diffuse
        const float diffuse =
            clamp(self->diffuse * get1f(self->diffuseMap, dg, 1.f));
        const vec3f diffuseColor = baseColor * diffuse;
        const float backlight = self->thin
            ? clamp(
                self->backlight * get1f(self->backlightMap, dg, 1.f), 0.f, 2.f)
            : 0.f;
        const float diffuseTransmission = backlight * 0.5f;
        const float diffuseReflection = 1.f - diffuseTransmission;

        if (diffuseReflection > EPS) {
          if (roughness < EPS)
            plasticBsdf = Lambert_create(ctx, baseFrame, diffuseColor);
          else
            plasticBsdf =
                OrenNayar_create(ctx, baseFrame, diffuseColor, roughness);
        }

        if (diffuseTransmission > EPS) {
          varying BSDF *uniform diffuseTransmissionBsdf =
              LambertTransmission_create(ctx, baseFrame, diffuseColor);

          if (plasticBsdf) {
            varying BSDF *uniform mixBsdf = MultiBSDF_create(ctx);
            MultiBSDF_add(
                mixBsdf, plasticBsdf, diffuseReflection, diffuseReflection);
            MultiBSDF_add(mixBsdf,
                diffuseTransmissionBsdf,
                diffuseTransmission,
                diffuseTransmission);
            plasticBsdf = mixBsdf;
          } else {
            plasticBsdf = diffuseTransmissionBsdf;
          }
        }

        // specular
        if (specular > EPS && abs(eta - 1.f) > EPS) {
          if (roughness < EPS) {
            plasticBsdf = DielectricLayer_create(ctx,
                baseFrame,
                plasticBsdf,
                eta,
                make_vec3f(1.f),
                1.f,
                specular);
          } else {
            plasticBsdf = MicrofacetDielectricLayer_create(ctx,
                baseFrame,
                plasticBsdf,
                eta,
                make_vec3f(1.f),
                1.f,
                roughness,
                anisotropy,
                specular);
          }
        }

        MultiBSDF_add(
            baseBsdf, plasticBsdf, plastic, plastic * max(diffuse, specular));
      }

      // glass base
      const float glass = dielectric * transmission * specular;
      if (glass > EPS) {
        varying BSDF *varying glassBsdf = NULL;

        if (abs(eta - 1.f) > EPS) {
          if (!self->thin) {
            // solid
            // use microfacet if textured due to different
            // transparent shadow behavior
            if (roughness < EPS && !valid(self->roughnessMap))
              glassBsdf = Dielectric_create(ctx, baseFrame, eta);
            else
              glassBsdf = MicrofacetDielectric_create(
                  ctx, baseFrame, eta, roughness, anisotropy);
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

            if (roughness < EPS)
              glassBsdf =
                  ThinDielectric_create(ctx, baseFrame, eta, attenuation);
            else
              glassBsdf = ThinMicrofacetDielectric_create(
                  ctx, baseFrame, eta, roughness, anisotropy, attenuation);
          }
        } else {
          glassBsdf = Transmission_create(ctx, baseFrame, make_vec3f(1.f));
        }

        MultiBSDF_add(baseBsdf, glassBsdf, glass, glass);
      }
    }

    // conductor base
    const float conductor = metallic * specular;
    if (conductor > EPS) {
      const vec3f edgeColor = clamp(
          self->edgeColor * get3f(self->edgeColorMap, dg, make_vec3f(1.f)));

      Fresnel *uniform fresnel =
          FresnelConductorArtistic_create(ctx, baseColor, edgeColor);
      varying BSDF *varying conductorBsdf;
      if (roughness < EPS)
        conductorBsdf = Conductor_create(ctx, baseFrame, fresnel);
      else
        conductorBsdf = MicrofacetConductor_create(
            ctx, baseFrame, fresnel, roughness, anisotropy);

      MultiBSDF_add(baseBsdf, conductorBsdf, conductor, conductor);
    }

    bsdf = baseBsdf;

    // coatings
    varying linear3f *uniform coatFrame = frame;

    // clear coat
    const float coat = clamp(self->coat * get1f(self->coatMap, dg, 1.f));
    if (coat > EPS) {
      float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
      if (coatIor < 1.f)
        coatIor = rcp(coatIor);
      float coatEta = fromOutside ? self->outsideMedium.ior * rcp(coatIor)
                                  : coatIor * rcp(self->outsideMedium.ior);
      coatEta =
          clamp(coatEta, 1.f / 3.f, 3.f); // clamp to common range due to LUTs

      if (abs(coatEta - 1.f) > EPS) {
        const vec3f coatColor = clamp(
            self->coatColor * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
        const float coatThickness = max(
            self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f), 0.f);
        const float coatRoughness =
            clamp(self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f));

        if (valid(self->coatNormalMap)) {
          coatFrame = LinearSpace3f_create(ctx,
              makeShadingFrame(dg,
                  self->coatNormalMap,
                  self->coatNormalRot,
                  self->coatNormal));
        }

        if (coatRoughness < EPS) {
          bsdf = DielectricLayer_create(
              ctx, coatFrame, bsdf, coatEta, coatColor, coatThickness, coat);
        } else {
          bsdf = MicrofacetDielectricLayer_create(ctx,
              coatFrame,
              bsdf,
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
    const float sheen = clamp(self->sheen * get1f(self->sheenMap, dg, 1.f));
    if (sheen > EPS) {
      vec3f sheenColor = clamp(
          self->sheenColor * get3f(self->sheenColorMap, dg, make_vec3f(1.f)));
      const float sheenTint =
          clamp(self->sheenTint * get1f(self->sheenTintMap, dg, 1.f));
      sheenColor = lerp(sheenTint, sheenColor, baseColor);
      const float sheenRoughness =
          clamp(self->sheenRoughness * get1f(self->sheenRoughnessMap, dg, 1.f));

      bsdf = MicrofacetSheenLayer_create(
          ctx, coatFrame, bsdf, sheenColor, sheenRoughness, sheen);
    }
  }

  // cut-out transparency
  const float transparency = 1.f - opacity;
  if (transparency > EPS) {
    varying BSDF *uniform transparencyBsdf =
        Transmission_create(ctx, frame, make_vec3f(transparency));

    if (bsdf) {
      varying BSDF *uniform mixBsdf = MultiBSDF_create(ctx);
      MultiBSDF_add(mixBsdf, bsdf, opacity, opacity);
      MultiBSDF_add(mixBsdf, transparencyBsdf, 1.f, transparency);
      bsdf = mixBsdf;
    } else {
      bsdf = transparencyBsdf;
    }
  }

  return bsdf;
}

vec3f Principled_getTransparency(const uniform Material *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Principled *uniform self =
      (const uniform Principled *uniform)material;
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
  if (opacity > EPS) {
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
        const float roughness =
            clamp(self->roughness * get1f(self->roughnessMap, dg, 1.f));

        if (!self->thin) {
          // solid
          // use microfacet if textured due to different
          // transparent shadow behavior
          if (roughness < EPS && !valid(self->roughnessMap))
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

          if (roughness < EPS)
            T = ThinDielectric_getTransparency(cosThetaO, eta, attenuation);
          else
            T = ThinMicrofacetDielectric_getTransparency(
                cosThetaO, eta, roughness, attenuation);
        }
      } else {
        T = make_vec3f(1.f);
      }

      if (reduce_max(T) > 0.f) {
        // clear coat
        const float coat = clamp(self->coat * get1f(self->coatMap, dg, 1.f));
        if (coat > EPS) {
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
            const float coatRoughness = clamp(
                self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f));

            if (coatRoughness < EPS) {
              T = T
                  * DielectricLayer_getTransparency(
                      cosThetaO, coatEta, coatColor, coatThickness, coat);
            } else {
              T = T
                  * MicrofacetDielectricLayer_getTransparency(cosThetaO,
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
        const float sheen = clamp(self->sheen * get1f(self->sheenMap, dg, 1.f));
        if (sheen > EPS) {
          const float sheenRoughness = clamp(
              self->sheenRoughness * get1f(self->sheenRoughnessMap, dg, 1.f));

          T = T
              * MicrofacetSheenLayer_getTransparency(
                  cosThetaO, sheenRoughness, sheen);
        }

        T = T * glass;
      }
    }
  }

  // cut-out transparency
  T = T * opacity + transparency;

  return T;
}

void Principled_selectNextMedium(const uniform Material *uniform super,
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

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Principled_getBSDF_addr()
{
  return Principled_getBSDF;
}

export void *uniform Principled_getTransparency_addr()
{
  return Principled_getTransparency;
}

export void *uniform Principled_selectNextMedium_addr()
{
  return Principled_selectNextMedium;
}
