// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
// AO Renderer
#include "Material.ih"
#include "surfaces.ih"

inline vec4f getSurfaceColor(const DifferentialGeometry &dg)
{
  const AOMaterial *mat = (const AOMaterial *)dg.material;

  vec3f surfaceColor = make_vec3f(1.f);
  float opacity = 1.f;
  foreach_unique (m in mat) {
    if (m != NULL && m->super.type == RENDERER_TYPE_AO) {
      surfaceColor = m->Kd;
      if (valid(m->KdMap)) {
        vec4f Kd_from_map = get4f(m->KdMap, dg);
        surfaceColor = surfaceColor * make_vec3f(Kd_from_map);
        opacity *= Kd_from_map.w;
      }
      opacity *= m->d * get1f(m->dMap, dg, 1.f);
    }
  }

  return make_vec4f(surfaceColor * make_vec3f(dg.color), opacity * dg.color.w);
}

SSI computeShading(const AORenderer *uniform self,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    const DifferentialGeometry &dg,
    ScreenSample &sample)
{
  SSI retval;

  const vec4f surfaceColor = getSurfaceColor(dg);

  const vec3f color = make_vec3f(surfaceColor);
  const float opacity = surfaceColor.w;
  const float eyeLightIntensity = absf(dot(dg.Ns, sample.ray.dir)) * opacity;

  float intensity = eyeLightIntensity;
  if (self->aoSamples > 0 && self->aoIntensity > 0.01f) {
    const float ao =
        computeAO(world, dg, self->aoSamples, self->aoRadius, sample.sampleID);
    intensity *= (1.0f - self->aoIntensity) + (ao * self->aoIntensity);
  }

  retval.shadedColor = make_vec4f(color * intensity, opacity);
  retval.albedo = color;

  return retval;
}
