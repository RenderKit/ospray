// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/GeometryLight.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerUtil.ih"

#include "render/Material.ih"

#include "common/Instance.ih"
#include "geometry/GeometricModel.ih"
#include "math/AffineSpace.ih"
#include "math/Distribution1D.ih"
// c++ shared
#include "GeometryLightShared.h"

vec3f evaluateGeometryLights(const PathContext &pathContext,
    const PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    Ray &ray)
{
  vec3f L = make_vec3f(0.f);
  Material *material = (Material *)pathVertex.dg.material;
  foreach_unique (m in material) {
    if (m != NULL && reduce_max(m->emission) > 0.f) {
      // at the moment we only support uniform light selection
      // therefore we can evaluate the selection PDF directly
      float lightSelectionPDF = 0.0f;
      if (pathContext.numLights > 0) {
        lightSelectionPDF =
            (1.0f / (float)pathContext.numLights) * pathContext.numLightSamples;
      }
      // convert pdf wrt. area to pdf wrt. solid angle
      const float cosd = dot(pathVertex.dg.Ng, ray.dir);
      const float lePdf = pathVertex.dg.areaPDF * sqr(ray.t) / abs(cosd);
      L = L
          + pathState.throughput * m->emission
              * misHeuristic(
                  pathState, lastVertex.pdf_w, lightSelectionPDF * lePdf);
    }
  }
  return L;
}

// Implementation /////////////////////////////////////////////////////////////

Light_SampleRes GeometryLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time)
{
  const GeometryLight *uniform self = (GeometryLight * uniform) super;
  const GeometricModel *uniform model = self->model;
  const Geometry *uniform geo = model->geom;
  Light_SampleRes res;

  // choose primitive
  const Sample1D sample =
      Distribution1D_sample(self->numPrimitives, self->distribution, 0, s.x);
  const int32 primID = self->primIDs[sample.idx];

  // XXX have to rescale s.x
  const vec2f ns = make_vec2f(sample.frac, s.y);
  // sample position on primitive
  // TODO maybe should already be done wrt. solid angle
  SampleAreaRes as;
  const Instance *uniform instance = self->super.instance;
  assert(instance);
  if (instance->motionBlur)
    foreach_unique (utime in time) {
      const uniform affine3f xfm = Instance_getTransform(instance, utime);
      as = geo->sampleArea(geo, primID, xfm, rcp(xfm), ns, time);
    }
  else {
    as = geo->sampleArea(
        geo, primID, instance->xfm, instance->rcp_xfm, ns, time);
  }
  // note that sample.pdf/primitives * sampleArea(worldspace).pdf == self->pdf

  // extant light vector from the hit point
  const vec3f dir = as.pos - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist - calcEpsilon(as.pos, dist);

  // convert to pdf wrt. solid angle
  const float cosd = dot(as.normal, res.dir);
  res.pdf = self->pdf * sqr(dist) / abs(cosd);

  // get radiance
  Material *mat =
      (Material *)GeometricModel_getMaterial(model, dg.renderer, primID);
  const vec3f radiance = mat ? mat->emission : make_vec3f(0.f);

  res.weight = radiance * rcp(res.pdf);

  return res;
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform GeometryLight_sample_addr()
{
  return GeometryLight_sample;
}
