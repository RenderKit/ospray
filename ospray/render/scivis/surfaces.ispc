// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ispc device
#include "math/random.ih"
#include "math/sampling.ih"
// SciVis renderer
#include "SciVisMaterial.ih"
#include "surfaces.ih"

inline vec4f getSurfaceColor(const DifferentialGeometry &dg)
{
  const SciVisMaterial *mat = (const SciVisMaterial *)dg.material;

  vec3f surfaceColor = make_vec3f(1.f);
  float opacity = 1.f;
  if (mat) {
    foreach_unique (m in mat) {
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

inline float computeAO(const uniform SciVis *uniform self,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    const varying vec3i &sampleID,
    const varying DifferentialGeometry &dg)
{
  const uniform int &sampleCnt = self->aoSamples;
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, 0x290374, (fb->size.x * sampleID.y) + sampleID.x);
  const vec2f rot = RandomTEA__getFloats(rng);

  int hits = 0;

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    // Use base-5 and base-7 Halton to avoid correlation with sample position
    const vec2f halton = make_vec2f(
        PermHalton_sample5(accumID + i), PermHalton_sample7(accumID + i));
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.01f) { // check below surface
      hits++;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, self->aoRadius);
    if (isOccluded(world, ao_ray))
      hits++;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}

///////////////////////////////////////////////////////////////////////////////

SSI computeShading(const SciVis *uniform self,
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

  const uniform bool doAO = self->aoSamples > 0 && self->aoIntensity > 0.01f;

  const float ao = doAO ? computeAO(self, fb, world, sample.sampleID, dg) : 1.f;

  const float aoIntensity = doAO ? self->aoIntensity : 0.f;

  const float intensity =
      ((1 - aoIntensity) + (ao * aoIntensity)) * eyeLightIntensity;

  retval.shadedColor = make_vec4f(color * intensity, opacity);
  retval.albedo = color;

  return retval;
}
