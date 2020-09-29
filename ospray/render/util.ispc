// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/World.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"

uniform z_order_t z_order;
uniform bool z_order_initialized = false;

void precomputedZOrder_create()
{
  for (uniform uint32 i = 0; i < TILE_SIZE * TILE_SIZE; i++) {
    deinterleaveZOrder(i, &z_order.xs[i], &z_order.ys[i]);
    z_order.xyIdx[i] = z_order.xs[i] | (z_order.ys[i] << 16);
  }

  z_order_initialized = true;
}

float computeAO(const World *uniform world,
    const varying DifferentialGeometry &dg,
    const uniform int sampleCnt,
    const uniform float aoRadius,
    const varying vec3i &sampleID)
{
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, sampleID.x, sampleID.y);
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
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, aoRadius);
    if (isOccluded(world, ao_ray))
      hits++;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}
