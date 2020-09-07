// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
// SciVis renderer
#include "SciVisMaterial.ih"
#include "lights/Light.ih"
#include "surfaces.ih"

struct SciVisBSDF
{
  vec3f diffuse;
  vec3f specular;
  float shininess;
  float opacity;
  vec3f albedo;
};

inline SciVisBSDF evalMaterial(const DifferentialGeometry &dg)
{
  const SciVisMaterial *mat = (const SciVisMaterial *)dg.material;

  // defaults
  SciVisBSDF res;
  res.diffuse = make_vec3f(dg.color);
  res.specular = make_vec3f(0.0f);
  res.shininess = 10.0f;
  res.opacity = dg.color.w;

  if (mat) {
    foreach_unique (m in mat) {
      res.opacity *= m->d * get1f(m->dMap, dg, 1.f);

      res.diffuse = res.diffuse * m->Kd;
      if (valid(m->KdMap)) {
        vec4f Kd_from_map = get4f(m->KdMap, dg);
        res.diffuse = res.diffuse * make_vec3f(Kd_from_map);
        res.opacity *= Kd_from_map.w;
      }
      res.diffuse = res.diffuse * res.opacity;
      res.specular = res.opacity * m->Ks * get3f(m->KsMap, dg, make_vec3f(1.f));
      res.shininess = m->Ns * get1f(m->NsMap, dg, 1.0f);
    }
  }

  res.albedo = res.diffuse + res.specular; // before normalization!

  // convert from Phong exponent to Blinn
  res.shininess *= 4.0f;
  // BRDF normalization
  res.diffuse = res.diffuse * one_over_pi;
  res.specular = res.specular
      * ((res.shininess + 2.f) * one_over_four_pi
          * rcp(2.f - exp(res.shininess * -lntwo_over_two)));

  return res;
}

vec3f directIllumination(const uniform SciVis *uniform self,
    const World *uniform world,
    const varying DifferentialGeometry &dg,
    const varying SciVisBSDF &bsdf,
    const varying vec3f &inDir,
    const float time)
{
  vec3f color = make_vec3f(0.f);
  if (!world->scivisData.lights)
    return color;

  // calculate shading for all lights
  for (uniform int i = 0; i < world->scivisData.numLights; i++) {
    const uniform Light *uniform l = world->scivisData.lights[i];
    const vec2f s = make_vec2f(0.0f); // sample center of area lights
    const Light_SampleRes light = l->sample(l, dg, s);

    if (reduce_max(light.weight) > 0.f) { // any potential contribution?
      const float cosNL = dot(light.dir, dg.Ns);
      if (cosNL < 0.0f)
          continue;

      const vec3f H = normalize(light.dir - inDir);
      const float cosNH = dot(H, dg.Ns);
      const vec3f specular = bsdf.specular * powf(cosNH, bsdf.shininess);
      const vec3f diffuse = bsdf.diffuse * cosNL;
      const vec3f light_contrib = (diffuse + specular) * light.weight;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > self->super.minContribution) {
          vec3f P = dg.P;
          if (dot(dg.Ng, light.dir) < 0.0f)
            P = P - (2.f * dg.epsilon) * dg.Ng;

          Ray shadowRay;
          setRay(shadowRay, P, light.dir, 0.0f, light.dist, time);

          if (!isOccluded(world, shadowRay))
            color = color + light_contrib;
        }
      } else {
        color = color + light_contrib;
      }
    }
  }

  return color;
}

SSI computeShading(const SciVis *uniform self,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    const DifferentialGeometry &dg,
    ScreenSample &sample,
    const varying vec3f &inDir,
    const float time)
{
  SSI retval;

  const SciVisBSDF bsdf = evalMaterial(dg);
  retval.albedo = bsdf.albedo;

  vec3f color = directIllumination(self, world, dg, bsdf, inDir, time);

  float ao = 1.f;
  if (self->aoSamples > 0 && reduce_max(world->scivisData.aoColorPi) > 0.01f)
    ao = computeAO(world, dg, self->aoSamples, self->aoRadius, sample.sampleID);

  color = color + bsdf.diffuse * ao * world->scivisData.aoColorPi;

  retval.shadedColor = make_vec4f(color, bsdf.opacity);

  return retval;
}
