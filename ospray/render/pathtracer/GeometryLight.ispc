// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "math/Distribution1D.ih"
#include "render/pathtracer/materials/Material.ih"

/* The GeometryLight is a proxy object fulfilling the Light.ih API.
   It is generated internally for each emitting geometry instance to facilitate
   explicit importance sampling (currently wrt. the surface area).
*/

struct GeometryLight
{
  Light super; // inherited light fields

  vec3f radiance; // RGB color and intensity of the GeometryLight
  Geometry *geo; // underlying (instanced) geometry
  affine3f xfm; // instance transform (obj2world)
  affine3f rcp_xfm; // inverse instance transform (world2obj)
  float *distribution; // pdf over primitives proportional to (world-space) area 
  float pdf; // probability density to sample point on surface := 1/area
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

Light_SampleRes GeometryLight_sample(const uniform Light* uniform super,
                                     const DifferentialGeometry& dg,
                                     const vec2f& s)
{
  const GeometryLight* uniform self = (GeometryLight* uniform)super;
  const Geometry* uniform geo = self->geo;
  Light_SampleRes res;

  // choose primitive
  const Sample1D sample = Distribution1D_sample(geo->primitives, self->distribution, 0, s.x);

  // XXX have to rescale s.x
  const vec2f ns = make_vec2f(sample.frac, s.y);
  // sample position on primitive
  // TODO maybe should already be done wrt. solid angle
  const SampleAreaRes as = geo->sampleArea(geo, sample.idx, self->xfm, self->rcp_xfm, ns);
  // note that sample.pdf/primitives * sampleArea(worldspace).pdf == self->pdf

  // extant light vector from the hit point
  const vec3f dir = as.pos - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(as.normal, res.dir);
  res.pdf = self->pdf * sqr(dist) / abs(cosd);

  res.weight = self->radiance * rcp(res.pdf);

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void* uniform GeometryLight_create(void* uniform _geo
    , const uniform affine3f &xfm
    , const uniform affine3f &rcp_xfm
    , float *uniform areaPDF
    )
{
  Geometry *uniform geo = (Geometry *uniform)_geo;

  if (!geo->sampleArea || !geo->getAreas) {
    // set pdf to zero, indicating that this emissive geometry cannot be
    // sampled, but only be hit (and thus MIS weight for BSDF samples will be 1)
    *areaPDF = 0.f;
    return NULL;
  }

  GeometryLight* uniform self = uniform new GeometryLight;

  Light_Constructor(&self->super);
  self->super.sample = GeometryLight_sample;
  self->super.eval = NULL; // geometry lights are hit and explicitely handled
  self->geo = geo;
  self->xfm = xfm;
  self->rcp_xfm = rcp_xfm;

  PathTraceMaterial *uniform mat = (PathTraceMaterial *uniform)geo->material;
  self->radiance = mat->emission;

  const uniform int32 prims = geo->primitives;
  self->distribution = uniform new uniform float[prims];
  geo->getAreas(geo, xfm, self->distribution);
  self->pdf = 1.f/Distribution1D_create(prims, self->distribution);
  *areaPDF = self->pdf;

  return self;
}

export void GeometryLight_destroy(void* uniform _self)
{
  GeometryLight* uniform self = (GeometryLight* uniform)_self;
  if (self)
    delete[] self->distribution;
  delete self;
}
