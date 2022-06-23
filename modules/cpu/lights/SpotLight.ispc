// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntensityDistribution.ih"
#include "Light.ih"
#include "common/Instance.ih"
#include "math/sampling.ih"

// state that changes under transformations
struct SpotLightDynamic
{
  vec3f position;
  vec3f direction;
  vec3f c0;
  vec3f c90;
};

struct SpotLight
{
  Light super;
  vec3f intensity; // RGB radiative intensity of the SpotLight
  vec3f radiance; // emitted RGB radiance
  float cosAngleMax; // Angular limit of the spot in an easier to use form:
                     // cosine of the half angle in radians
  float cosAngleScale; // 1/(cos(border of the penumbra area) - cosAngleMax);
                       // positive
  float radius; // defines the size of the DiskLight represented using
                // the (extended) SpotLight
  float innerRadius; // defines the size of the inner cut out of the DiskLight
                     // to represent a RingLight
  float areaPdf; // pdf of disk/ring with radius/innerRadius
  IntensityDistribution intensityDistribution;
  SpotLightDynamic pre; // un- or pre-transformed state
};

// Implementation
//////////////////////////////////////////////////////////////////////////////
inline void Transform(const uniform SpotLightDynamic &pre,
    const uniform affine3f xfm,
    uniform SpotLightDynamic &dyn)
{
  dyn.position = xfmPoint(xfm, pre.position);
  dyn.direction = normalize(xfmVector(xfm, pre.direction));
  dyn.c90 = normalize(cross(xfmVector(xfm, pre.c0), dyn.direction));
  dyn.c0 = cross(dyn.direction, dyn.c90);
}

inline float AngularAttenuation(const SpotLight *uniform self, float cosAngle)
{
  return clamp((cosAngle - self->cosAngleMax) * self->cosAngleScale);
}

inline Light_SampleRes Sample(const SpotLight *uniform self,
    const uniform SpotLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  Light_SampleRes res;

  // extant light vector from the hit point
  res.dir = dyn.position - dg.P;

  if (self->radius > 0.0f) {
    uniform linear3f l2w;
    l2w.vx = dyn.c0;
    l2w.vy = dyn.c90;
    l2w.vz = dyn.direction;
    res.dir =
        l2w * uniformSampleRing(self->radius, self->innerRadius, s) + res.dir;
  }

  const float dist2 = dot(res.dir, res.dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  res.dir = res.dir * invdist;
  res.dist = dist2 * invdist;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(dyn.direction, res.dir);

  float weight = AngularAttenuation(self, cosAngle);
  if (self->intensityDistribution.lid)
    weight *= IntensityDistribution_eval(
        &self->intensityDistribution, dyn.c0, dyn.c90, cosAngle, res.dir);
  if (self->radius > 0.0f) {
    // convert area PDF to solid angle PDF
    res.pdf = self->areaPdf * (dist2 / abs(cosAngle));
    if (self->intensityDistribution.lid) {
      // when an light distribution function is used we want to
      // remove the cosine term. To avoid numerical issues
      // at cosineAngle = 0 we use the fact that the division
      // of radiance with the cosine cancels out.
      weight /= self->areaPdf * dist2;
    } else {
      weight /= res.pdf;
    }
    res.weight = self->radiance * weight;
  } else {
    res.pdf = inf; // we always take this sample
    if (!self->intensityDistribution.lid) {
      // if the spotlight does not use a measured LDF we
      // simulate Lambertian behavior by multiplication with cosineAngle
      weight *= abs(cosAngle);
    }
    // convert from intensity to radiance by attenuating by distance^2;
    // attenuate by angle
    res.weight = self->intensity * (sqr(invdist) * weight);
  }
  return res;
}

Light_SampleRes SpotLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  assert(self);
  return Sample(self, self->pre, dg, s);
}

Light_SampleRes SpotLight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    uniform SpotLightDynamic dyn;
    Transform(self->pre, xfm, dyn);
    res = Sample(self, dyn, dg, s);
  }
  return res;
}

inline Light_EvalRes Eval(const SpotLight *uniform self,
    const uniform SpotLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (self->radius > 0.f) {
    // intersect ring
    const float cosAngle = -dot(dyn.direction, dir);
    if (cosAngle > self->cosAngleMax) { // inside illuminated cone?
      const vec3f vp = dg.P - dyn.position;
      const float t = dot(vp, dyn.direction) * rcp(cosAngle);
      if (t > minDist & t <= maxDist) {
        const vec3f vd = vp + t * dir;
        const float d2 = dot(vd, vd);
        // inside ring?
        if (and(d2<sqr(self->radius), d2> sqr(self->innerRadius))) {
          float attenuation = AngularAttenuation(self, cosAngle);
          if (self->intensityDistribution.lid) {
            attenuation *= IntensityDistribution_eval(
                &self->intensityDistribution, dyn.c0, dyn.c90, cosAngle, dir);
            // convert from intensity to radiance by canceling the the cosine
            // term introduced by the Lambertian area light
            attenuation /= abs(cosAngle);
          }

          // calculate the attenuated emitted radiance
          res.radiance = self->radiance * attenuation;

          // convert area PDF to solid angle PDF
          res.pdf = self->areaPdf * (sqr(t) / abs(cosAngle));
        }
      }
    }
  }
  return res;
}

Light_EvalRes SpotLight_eval(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  assert(self);
  return Eval(self, self->pre, dg, dir, minDist, maxDist);
}

Light_EvalRes SpotLight_eval_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    uniform SpotLightDynamic dyn;
    Transform(self->pre, xfm, dyn);
    res = Eval(self, dyn, dg, dir, minDist, maxDist);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void SpotLight_set(void *uniform super,
    const uniform vec3f &position,
    const uniform vec3f &direction,
    const uniform vec3f &c0,
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

  self->pre.position = position;
  self->pre.direction = normalize(direction);

  IntensityDistribution_set(&self->intensityDistribution, lid, size);

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    self->pre.c0 = c0;
    if (instance->motionBlur) {
      self->super.sample = SpotLight_sample_instanced;
      self->super.eval = SpotLight_eval_instanced;
    } else
      Transform(self->pre, instance->xfm, self->pre);
  } else {
    self->pre.c90 = normalize(cross(c0, self->pre.direction));
    self->pre.c0 = cross(self->pre.direction, self->pre.c90);
  }

  self->radiance = radiance;
  self->intensity = intensity;
  self->cosAngleMax = cosAngleMax;
  self->cosAngleScale = cosAngleScale;
  self->radius = radius;
  self->innerRadius = innerRadius;
  self->areaPdf = uniformSampleRingPDF(radius, innerRadius);
}

export void *uniform SpotLight_create()
{
  SpotLight *uniform self = uniform new SpotLight;

  Light_Constructor(&self->super);
  self->super.sample = SpotLight_sample;
  self->super.eval = SpotLight_eval;

  SpotLight_set(self,
      make_vec3f(0.f),
      make_vec3f(0.f, 0.f, 1.f),
      make_vec3f(0.f, 1.f, 0.f),
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
