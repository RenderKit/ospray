// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "lights/Light.ih"
#include "math/AffineSpace.ih"
#include "geometry/Geometry.ih"
#include "render/pathtracer/materials/Material.ih"

/* The GeometryLight is a proxy object fulfilling the Light.ih API.
   It is generated internally for each emitting geometry to facilitate
   explicit importance sampling (currently wrt. the surface area).
*/

struct GeometryLight
{
  Light super; // inherited light fields

  AreaSampler *areaSampler;
  vec3f radiance; // RGB color and intensity of the GeometryLight
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

Light_SampleRes GeometryLight_sample(const uniform Light* uniform super,
                                     const DifferentialGeometry& dg,
                                     const vec2f& s)
{
  const GeometryLight* uniform self = (GeometryLight* uniform)super;
  Light_SampleRes res;

  // sample position on light
  // TODO should already be wrt. solid angle
  AreaSampler_SampleRes as = self->areaSampler->sample(self->areaSampler, s);

  // extant light vector from the hit point
  const vec3f dir = as.pos - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(as.normal, res.dir);
  res.pdf = self->areaSampler->pdf * sqr(dist) / abs(cosd);

  res.weight = self->radiance * rcp(res.pdf);

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void* uniform GeometryLight_create(void* uniform _geo,
    const uniform affine3f &xfm, float *uniform areaPDF)
{
  GeometryLight* uniform self = uniform new GeometryLight;

  Light_Constructor(&self->super);
  self->super.sample = GeometryLight_sample;
  self->super.eval = NULL; // geometry lights are hit and explicitely handled

  Geometry *uniform geo = (Geometry *uniform)_geo;
  PathTraceMaterial *uniform mat = (PathTraceMaterial *uniform)geo->material;

  if (geo->generateAreaSampler)
    self->areaSampler = geo->generateAreaSampler(geo, xfm);
  else {
    print("#osp:pt generateAreaSampler not implemented! Cannot use importance "
        "sampling for that geometry with emissive material!\n");
    // set pdf to inf, indicating that this emissive geometry cannot be
    // sampled, but hit (and thus MIS weight will be 1)
    *areaPDF = inf;
    return NULL;
  }
  self->radiance = mat->emission;
  *areaPDF = self->areaSampler->pdf;

  return self;
}

export void GeometryLight_destroy(void* uniform _self)
{
  uniform GeometryLight* uniform self = (uniform GeometryLight* uniform)_self;
  self->areaSampler->destroy(self->areaSampler);
  delete self;
}
