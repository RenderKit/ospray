// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/FeatureFlagsEnum.h"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/ScreenSample.ih"
#include "render/materials/OBJ.ih"
#include "render/util.ih"
#include "texture/TextureParam.ih"
// AO Renderer
#include "surfaces.ih"
// c++ shared
#include "render/materials/OBJShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline vec4f getSurfaceColor(
    const DifferentialGeometry &dg, const uniform FeatureFlagsHandler &ffh)
{
  const OBJ *mat = (const OBJ *)dg.material;

  vec3f surfaceColor = make_vec3f(1.f);
  float opacity = 1.f;

  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  const uniform bool fft = ff.other & FFO_TEXTURE_IN_MATERIAL;

#ifdef OSPRAY_TARGET_SYCL
  {
    const OBJ *m = mat;
#else
  foreach_unique (m in mat) {
#endif
    if (m != NULL && m->super.type == MATERIAL_TYPE_OBJ) {
      surfaceColor = m->Kd;
      if (valid(m->KdMap) && fft) {
        vec4f Kd_from_map = get4f(m->KdMap, dg);
        surfaceColor = surfaceColor * make_vec3f(Kd_from_map);
        opacity *= Kd_from_map.w;
      }
      opacity *= m->d * (fft ? get1f(m->dMap, dg, 1.f) : 1.f);
    }
  }

  return make_vec4f(surfaceColor * make_vec3f(dg.color), opacity * dg.color.w);
}

SYCL_EXTERNAL SSI AORenderer_computeShading(const AORenderer *uniform self,
    const World *uniform world,
    const DifferentialGeometry &dg,
    ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  SSI retval;

  const vec4f surfaceColor = getSurfaceColor(dg, ffh);

  const vec3f color = make_vec3f(surfaceColor);
  const float opacity = surfaceColor.w;
  const float eyeLightIntensity = absf(dot(dg.Ns, sample.ray.dir)) * opacity;

  float intensity = eyeLightIntensity;
  if (self->aoSamples > 0 && self->aoIntensity > 0.01f) {
    const float ao = computeAO(&self->super,
        world,
        dg,
        self->aoSamples,
        self->aoRadius,
        sample.sampleID,
        ffh);
    intensity *= (1.0f - self->aoIntensity) + (ao * self->aoIntensity);
  }

  retval.shadedColor = make_vec4f(color * intensity, opacity);
  retval.albedo = color;

  return retval;
}

OSPRAY_END_ISPC_NAMESPACE
