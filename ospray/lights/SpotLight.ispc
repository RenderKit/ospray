// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct SpotLight
{
  Light super;

  vec3f position; // Position of the SpotLight
  linear3f light2world; // coordinate frame, with vz == direction that the
                        // SpotLight is emitting
  linear3f world2light;
  vec3f intensity; // RGB color and intensity of the SpotLight
  float cosAngleMax; // Angular limit of the spot in an easier to use form:
                     // cosine of the half angle in radians
  float cosAngleScale; // 1/(cos(border of the penumbra area) - cosAngleMax);
                       // positive
  float radius; // defines the size of the DiskLight represented using
                // the (extended) SpotLight
  float innerRadius; // defines the size of the inner cut out of the DiskLight
                     // to represent a RingLight
  float areaPdf; // pdf of disk/ring with radius/innerRadius
  float *lid; // luminance intensity distribution
  vec2i size;
  vec2f scale;
};

// Implementation
//////////////////////////////////////////////////////////////////////////////
static float SpotLight_angularAttenuation(
    const SpotLight *uniform self, float cosAngle, const vec3f &dir)
{
  float angularAttenuation =
      clamp((cosAngle - self->cosAngleMax) * self->cosAngleScale);

  if (self->lid) {
    const vec3f localDir = self->world2light * dir;
    const float u = acos(cosAngle) * self->scale.x;
    const float u_frac = frac(u);
    const int u0 = u;
    const int u1 = (u0 + 2 >= self->size.x) ? self->size.x - 1 : u0 + 1;

    if (self->size.y == 1) { // 1D
      angularAttenuation *= lerp(u_frac, self->lid[u0], self->lid[u1]);
    } else { // 2D
      const float v = (atan2(localDir.y, localDir.x) + pi) * self->scale.y;
      const float v_frac = frac(v);
      const int v0 = v;
      const int v1 = (v0 + 1 >= self->size.y) ? 0 : v0 + 1;
      const int r0 = v0 * self->size.x;
      const int r1 = v1 * self->size.x;
      const float li0 = lerp(u_frac, self->lid[r0 + u0], self->lid[r0 + u1]);
      const float li1 = lerp(u_frac, self->lid[r1 + u0], self->lid[r1 + u1]);
      angularAttenuation *= lerp(v_frac, li0, li1);
    }
  }

  return angularAttenuation;
}

Light_SampleRes SpotLight_sample(
    const Light *uniform super, const DifferentialGeometry &dg, const vec2f &s)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  Light_SampleRes res;

  // extant light vector from the hit point
  res.dir = self->position - dg.P;

  if (self->radius > 0.)
    res.dir = self->light2world
            * uniformSampleRing(self->radius, self->innerRadius, s)
        + res.dir;

  const float dist2 = dot(res.dir, res.dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  res.dir = res.dir * invdist;
  res.dist = dist2 * invdist;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(self->light2world.vz, res.dir);

  if (self->radius > 0.) {
    // convert area PDF to solid angle PDF
    res.pdf = self->areaPdf * (dist2 / abs(cosAngle));
    // calculate the attenuated emitted radiance
    res.weight = self->super.radiance
        * SpotLight_angularAttenuation(self, cosAngle, res.dir);
    if (!self->lid) {
      res.weight = res.weight / res.pdf;
    } else {
      // when an light distribution function is used we want to
      // remove the cosine term. To avoid numerical issues
      // at cosineAngle = 0 we use the fact that the division
      // of radiance with the cosine cancels out.
      res.weight = res.weight / (self->areaPdf * dist2);
    }
  } else {
    res.pdf = inf; // we always take this sample
    // convert from intensity to radiance by attenuating by distance^2;
    // attenuate by angle
    res.weight = self->intensity
        * (sqr(invdist)
            * SpotLight_angularAttenuation(self, cosAngle, res.dir));
    // if the spotlight does not use a measured LDF we
    // simulate Lambertian behavior by multiplication with cosineAngle
    if (!self->lid) {
      res.weight = res.weight * abs(cosAngle);
    }
  }
  return res;
}

Light_EvalRes SpotLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (self->radius > 0.f) {
    // intersect ring
    const float cosAngle = -dot(dir, self->light2world.vz);
    if (cosAngle > self->cosAngleMax) { // inside illuminated cone?
      const vec3f vp = dg.P - self->position;
      const float t = dot(vp, self->light2world.vz) * rcp(cosAngle);
      if (t > minDist & t <= maxDist) {
        const vec3f vd = vp + t * dir;
        const float d2 = dot(vd, vd);
        // inside ring?
        if (and(d2 < sqr(self->radius), d2 > sqr(self->innerRadius))) {
          // calculate the attenuated emitted radiance
          res.radiance = self->super.radiance
              * SpotLight_angularAttenuation(self, cosAngle, dir);
          // since the light distribution function already includes
          // the cosine term we remove it from the  Lambertian area
          // light
          if (self->lid) {
            res.radiance = res.radiance / abs(cosAngle);
          }
          // convert area PDF to solid angle PDF
          res.pdf = self->areaPdf * (sqr(t) / abs(cosAngle));
        }
      }
    }
  }

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void SpotLight_set(void *uniform super,
    const uniform vec3f &position,
    const uniform linear3f &frame,
    const uniform vec3f &radiance,
    const uniform vec3f &intensity,
    uniform float cosAngleMax,
    uniform float cosAngleScale,
    uniform float radius,
    uniform float innerRadius,
    const uniform vec2i &size,
    float *uniform lid)
{
  SpotLight *uniform self = (SpotLight * uniform) super;
  self->position = position;
  self->light2world = frame;
  self->super.radiance = radiance;
  self->intensity = intensity;
  self->cosAngleMax = cosAngleMax;
  self->cosAngleScale = cosAngleScale;
  self->radius = radius;
  self->innerRadius = innerRadius;
  self->areaPdf = uniformSampleRingPDF(radius, innerRadius);
  self->lid = lid;
  if (self->lid) {
    self->world2light = rcp(frame);
    self->size = size;
    self->scale = make_vec2f(nextafter((size.x - 1) * one_over_pi, -1.0f),
        nextafter(self->size.y * one_over_two_pi, -1.0f));
  }
}

export void *uniform SpotLight_create()
{
  SpotLight *uniform self = uniform new SpotLight;

  Light_Constructor(&self->super);
  self->super.sample = SpotLight_sample;
  self->super.eval = SpotLight_eval;

  SpotLight_set(self,
      make_vec3f(0.f),
      make_LinearSpace3f_identity(),
      make_vec3f(1.f),
      make_vec3f(1.f),
      0.f,
      100.f,
      0.f,
      0.f,
      make_vec2i(0),
      NULL);

  return self;
}
