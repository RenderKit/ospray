// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "common/Instance.ih"
#include "math/sampling.ih"

struct SpotLight
{
  Light super;

  vec3f position; // Position of the SpotLight
  vec3f direction;
  vec3f c0;
  linear3f light2world; // coordinate frame, with vz == direction that the
                        // SpotLight is emitting
  linear3f world2light;
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
  float *lid; // luminance intensity distribution
  vec2i size;
  vec2f scale;
};

// Implementation
//////////////////////////////////////////////////////////////////////////////
inline float AngularAttenuation(const SpotLight *uniform self, float cosAngle)
{
  return clamp((cosAngle - self->cosAngleMax) * self->cosAngleScale);
}

inline float IntensityDistr(
    const SpotLight *uniform self, float cosAngle, const vec3f &localDir)
{
  const float u = acos(cosAngle) * self->scale.x;
  const float u_frac = frac(u);
  const int u0 = u;
  const int u1 = (u0 + 2 >= self->size.x) ? self->size.x - 1 : u0 + 1;

  float intensity;
  if (self->size.y == 1) { // 1D
    intensity = lerp(u_frac, self->lid[u0], self->lid[u1]);
  } else { // 2D
    const float v = (atan2(localDir.y, localDir.x) + pi) * self->scale.y;
    const float v_frac = frac(v);
    const int v0 = v;
    const int v1 = (v0 + 1 >= self->size.y) ? 0 : v0 + 1;
    const int r0 = v0 * self->size.x;
    const int r1 = v1 * self->size.x;
    const float li0 = lerp(u_frac, self->lid[r0 + u0], self->lid[r0 + u1]);
    const float li1 = lerp(u_frac, self->lid[r1 + u0], self->lid[r1 + u1]);
    intensity = lerp(v_frac, li0, li1);
  }
  return intensity;
}

inline Light_SampleRes Sample(const SpotLight *uniform self,
    const uniform vec3f &pos,
    const uniform linear3f &l2w,
    const uniform linear3f &w2l,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  Light_SampleRes res;

  // extant light vector from the hit point
  res.dir = pos - dg.P;

  if (self->radius > 0.)
    res.dir =
        l2w * uniformSampleRing(self->radius, self->innerRadius, s) + res.dir;

  const float dist2 = dot(res.dir, res.dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  res.dir = res.dir * invdist;
  res.dist = dist2 * invdist;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(l2w.vz, res.dir);

  if (self->radius > 0.) {
    // convert area PDF to solid angle PDF
    res.pdf = self->areaPdf * (dist2 / abs(cosAngle));
    // calculate the attenuated emitted radiance
    float attenuation = AngularAttenuation(self, cosAngle);
    if (!self->lid) {
      res.weight = (self->radiance * attenuation) / res.pdf;
    } else {
      // when an light distribution function is used we want to
      // remove the cosine term. To avoid numerical issues
      // at cosineAngle = 0 we use the fact that the division
      // of radiance with the cosine cancels out.
      attenuation *= IntensityDistr(self, cosAngle, w2l * res.dir);
      res.weight = (self->radiance * attenuation) / (self->areaPdf * dist2);
    }
  } else {
    res.pdf = inf; // we always take this sample
    float attenuation = AngularAttenuation(self, cosAngle);
    if (self->lid) {
      attenuation *= IntensityDistr(self, cosAngle, w2l * res.dir);
    } else {
      // if the spotlight does not use a measured LDF we
      // simulate Lambertian behavior by multiplication with cosineAngle
      attenuation *= abs(cosAngle);
    }
    // convert from intensity to radiance by attenuating by distance^2;
    // attenuate by angle
    res.weight = self->intensity * (sqr(invdist) * attenuation);
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
  return Sample(
      self, self->position, self->light2world, self->world2light, dg, s);
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

    // transform light into the world space
    const uniform vec3f pos = xfmPoint(xfm, self->position);
    const uniform vec3f dir = normalize(xfmVector(xfm, self->direction));
    const uniform vec3f c0 = normalize(xfmVector(xfm, self->c0));

    // prepare spotlight to world transformation matrix
    uniform linear3f l2w;
    l2w.vz = dir;
    l2w.vy = normalize(cross(c0, l2w.vz));
    l2w.vx = cross(l2w.vz, l2w.vy);

    // prepare world to spotlight transformation matrix,
    // this is orthonormal transformation so transposing is enough
    const uniform linear3f w2l = transposed(l2w);

    // sample light
    res = Sample(self, pos, l2w, w2l, dg, s);
  }
  return res;
}

inline Light_EvalRes Eval(const SpotLight *uniform self,
    const uniform vec3f &position,
    const uniform vec3f &direction,
    const uniform linear3f &w2l,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (self->radius > 0.f) {
    // intersect ring
    const float cosAngle = -dot(dir, direction);
    if (cosAngle > self->cosAngleMax) { // inside illuminated cone?
      const vec3f vp = dg.P - position;
      const float t = dot(vp, direction) * rcp(cosAngle);
      if (t > minDist & t <= maxDist) {
        const vec3f vd = vp + t * dir;
        const float d2 = dot(vd, vd);
        // inside ring?
        if (and(d2<sqr(self->radius), d2> sqr(self->innerRadius))) {
          float attenuation = AngularAttenuation(self, cosAngle);
          if (self->lid) {
            attenuation *= IntensityDistr(self, cosAngle, w2l * dir);

            // since the light distribution function already includes
            // the cosine term we remove it from the Lambertian area
            // light
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
  return Eval(self,
      self->position,
      self->direction,
      self->world2light,
      dg,
      dir,
      minDist,
      maxDist);
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

    // transform light into the world space
    const uniform vec3f position = xfmPoint(xfm, self->position);
    const uniform vec3f direction = normalize(xfmVector(xfm, self->direction));

    // prepare world to spotlight transformation matrix,
    // this is orthonormal transformation so transposing is enough
    const uniform vec3f c0 = normalize(xfmVector(xfm, self->c0));
    const uniform vec3f vy = normalize(cross(c0, direction));
    const uniform vec3f vx = cross(direction, vy);
    uniform linear3f w2l;
    w2l.vx = make_vec3f(vx.x, vy.x, direction.x);
    w2l.vy = make_vec3f(vx.y, vy.y, direction.y);
    w2l.vz = make_vec3f(vx.z, vy.z, direction.z);

    // evaluate light
    res = Eval(self, position, direction, w2l, dg, dir, minDist, maxDist);
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

  self->position = position;
  self->direction = normalize(direction);
  self->c0 = normalize(c0);

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    if (instance->motionBlur) {
      self->super.sample = SpotLight_sample_instanced;
      self->super.eval = SpotLight_eval_instanced;
    } else {
      self->position = xfmPoint(instance->xfm, self->position);
      self->direction = normalize(xfmVector(instance->xfm, self->direction));
      self->c0 = normalize(xfmVector(instance->xfm, self->c0));
    }
  }

  self->light2world.vz = self->direction;
  self->light2world.vy = normalize(cross(self->c0, self->light2world.vz));
  self->light2world.vx = cross(self->light2world.vz, self->light2world.vy);

  self->radiance = radiance;
  self->intensity = intensity;
  self->cosAngleMax = cosAngleMax;
  self->cosAngleScale = cosAngleScale;
  self->radius = radius;
  self->innerRadius = innerRadius;
  self->areaPdf = uniformSampleRingPDF(radius, innerRadius);
  self->lid = lid;
  if (self->lid) {
    self->world2light = rcp(self->light2world);
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
