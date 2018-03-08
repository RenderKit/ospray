// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

  Geometry *geo; // underlying (instanced) geometry
  affine3f xfm; // instance transform (obj2world)
  affine3f rcp_xfm; // inverse instance transform (world2obj)
  int32 numPrimitives; // number of emissive primitives
  int32 *primIDs; // IDs of emissive primitives to sample
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
  const Sample1D sample = Distribution1D_sample(self->numPrimitives, self->distribution, 0, s.x);
  const int32 primID = self->primIDs[sample.idx];

  // XXX have to rescale s.x
  const vec2f ns = make_vec2f(sample.frac, s.y);
  // sample position on primitive
  // TODO maybe should already be done wrt. solid angle
  const SampleAreaRes as = geo->sampleArea(geo, primID, self->xfm, self->rcp_xfm, ns);
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
  
  // get radiance
  const int32 matID = geo->getMaterialID(geo, primID);
  PathTraceMaterial *mat = (PathTraceMaterial*)geo->materialList[matID < 0 ? 0 : matID];
  const vec3f radiance = mat->emission;

  res.weight = radiance * rcp(res.pdf);

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export bool uniform GeometryLight_isSupported(void* uniform _geo)
{
  Geometry *uniform geo = (Geometry *uniform)_geo;
  return geo->sampleArea && geo->getAreas;
}

export void* uniform GeometryLight_create(void* uniform _geo
    , const uniform affine3f &xfm
    , const uniform affine3f &rcp_xfm
    , float *uniform areaPDF
    )
{
  Geometry *uniform geo = (Geometry *uniform)_geo;

  if (!GeometryLight_isSupported(geo)) {
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

  // create the list of emissive primitive IDs
  self->primIDs = uniform new uniform int32[geo->numPrimitives];
  uniform int32 numEmissivePrims = 0;
  foreach (primID = 0 ... geo->numPrimitives) {
    const int32 matID = geo->getMaterialID(geo, primID);
    PathTraceMaterial *mat = (PathTraceMaterial *)geo->materialList[matID < 0 ? 0 : matID];
    const vec3f emission = mat->emission;
    if (reduce_max(emission) > 0.f) {
       int offset = exclusive_scan_add(1);
       self->primIDs[numEmissivePrims + offset] = primID;
       numEmissivePrims += reduce_add(1);
    }
  }
  
  if (numEmissivePrims == 0) {
    // no emissive primitives, return NULL
    delete[] self->primIDs;
    *areaPDF = 0.f;
    return NULL;
  } else if (numEmissivePrims < geo->numPrimitives) {
    // save memory
    int32 *uniform newPrimIDs = uniform new uniform int32[numEmissivePrims];
    memcpy(newPrimIDs, self->primIDs, numEmissivePrims*sizeof(int32));
    delete[] self->primIDs;
    self->primIDs = newPrimIDs;
  }
  
  self->numPrimitives = numEmissivePrims;

  // create the sampling distribution
  // TODO: use emissive power instead of just area
  self->distribution = uniform new uniform float[numEmissivePrims];
  geo->getAreas(geo, self->primIDs, numEmissivePrims, xfm, self->distribution);
  self->pdf = 1.f/Distribution1D_create(numEmissivePrims, self->distribution);
  *areaPDF = self->pdf;

  return self;
}

export void GeometryLight_destroy(void* uniform _self)
{
  GeometryLight* uniform self = (GeometryLight* uniform)_self;
  if (self) {
    delete[] self->primIDs;
    delete[] self->distribution;
  }
  delete self;
}
