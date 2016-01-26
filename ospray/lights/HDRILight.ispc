// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Light.ih"
#include "ospray/math/LinearSpace.ih"
#include "ospray/math/Distribution2D.ih"
#include "ospray/math/sampling.ih"
#include "ospray/texture/Texture2D.ih"

struct HDRILight
{
  Light super;                        //!< inherited light fields

  linear3f light2world;               //!< Transformation from light space into world space
  linear3f world2light;               //!< Transformation from world space into light space
  Texture2D* uniform map;             //!< Environment map in latitude / longitude format
  float intensity;                    //!< Scaling factor for the map
  Distribution2D* uniform distribution; //!< The 2D distribution used to importance sample
  vec2f rcpSize;                      //!< precomputed 1/map.size
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample HDRILight_sample_dummy(const uniform Light *uniform,
                                           const varying DifferentialGeometry &,
                                           const varying vec2f &)
{
  LightSample sample;
  memset(&sample, 0, sizeof(LightSample));
  return sample;
}

varying LightSample HDRILight_sample(const uniform Light *uniform _self,
                                     const varying DifferentialGeometry &dg,
                                     const varying vec2f &s)
{
  uniform HDRILight *uniform self = (uniform HDRILight *uniform)_self;
  LightSample sample;

  Sample2D sample2d = Distribution2D_sample(self->distribution, s);

  const float phi = two_pi * sample2d.uv.x;
  const float theta = M_PI * sample2d.uv.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  sample.direction = self->light2world * localDir;

  sample.pdf = sample2d.pdf * rcp(two_pi * M_PI * sinTheta);

  sample.distance = inf;
  sample.radiance = get3f(self->map, sample2d.uv) * self->intensity / sample.pdf;

  return sample;
}

varying vec3f HDRILight_evalEnv(const uniform Light *uniform _self,
                                const varying vec3f &dir)
{
  uniform HDRILight *uniform self = (uniform HDRILight *uniform)_self;

  const vec3f localDir = self->world2light * dir;

  const float u = atan2(localDir.y, localDir.x) * one_over_two_pi;
  const float v = acos(localDir.z) * one_over_pi;

  return get3f(self->map, make_vec2f(u, v)) * self->intensity;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side HDRILight object
export void HDRILight_set(void *uniform _self,
                          const uniform linear3f &light2world,
                          void *uniform map,
                          const uniform float intensity)
{
  uniform HDRILight *uniform self = (uniform HDRILight *uniform)_self;

  if (self->distribution != NULL) {
    Distribution2D_destroy(self->distribution);
    self->distribution = NULL;
  }

  if (map) {
    self->light2world = light2world;
    self->world2light = rcp(light2world); 

    self->map = (uniform Texture2D *uniform)map;
    self->intensity = intensity;

    self->rcpSize = 1.f/self->map->sizef;
    const uniform int width  = self->map->size.x;
    const uniform int height = self->map->size.y;

    // calculate importance
    uniform float* uniform importance = uniform new uniform float[width*height];  
    for (uniform int y = 0; y < height; y++) {
      uniform float sinTheta = sin(M_PI * (y+0.5f) * self->rcpSize.y);
      for (uniform int x = 0; x < width; x++) {
        const uniform vec2f coord = make_vec2f(x, y)*self->rcpSize;
        const vec3f col = get3f(self->map, coord); // using bilinear filtering is indeed what we want
        const uniform vec3f colu = make_vec3f(extract(col.x, 0), extract(col.y, 0), extract(col.z, 0));
        importance[y*width+x] = sinTheta * luminance(colu);
      }
    }

    // create distribution
    self->distribution = Distribution2D_create(self->map->size, importance);
    // no delete[] importance: ownership was transferred to Distribution2D

    self->super.sample = HDRILight_sample;
    self->super.evalEnv = HDRILight_evalEnv;
  } else {
    self->super.sample = HDRILight_sample_dummy;
    self->super.evalEnv = defaultEvalEnv;
  }
}

//! Create an ispc-side HDRILight object
export void *uniform HDRILight_create(void *uniform cppEquivalent)
{
  uniform HDRILight *uniform self = uniform new uniform HDRILight;

  Light_Constructor(&self->super, cppEquivalent);
  self->super.sample = HDRILight_sample_dummy;
  self->distribution = NULL;

  HDRILight_set(self, make_LinearSpace3f_identity(), NULL, 1.f);

  return self;
}

//! Destroy an ispc-side HDRILight object
export void HDRILight_destroy(void *uniform _self)
{ 
  uniform HDRILight *uniform self = (uniform HDRILight *uniform)_self;
  Distribution2D_destroy(self->distribution);
  delete self;
}

