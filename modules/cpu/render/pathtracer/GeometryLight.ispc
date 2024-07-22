// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/GeometryLight.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerUtil.ih"

#include "render/Material.ih"
#include "render/MaterialDispatch.ih"
#include "texture/Texture2D.ih"
#include "texture/TextureParam.ih"

#include "common/Instance.ih"
#include "geometry/GeometricModel.ih"
#include "math/Distribution1D.ih"
#include "rkcommon/math/AffineSpace.ih"
// c++ shared
#include "GeometryLightShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec3f evaluateGeometryLights(const PathContext &pathContext,
    const PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    const Ray &ray,
    const uniform FeatureFlagsHandler &ffh)
{
  vec3f L = make_vec3f(0.f);
  Material *material = pathVertex.dg.material;

  foreach_unique (m in material) {
    vec3f emission = Material_dispatch_getEmission(m, pathVertex.dg, ffh);
    if (m != NULL && reduce_max(emission) > 0.f) {
      float misWeight = 1.0f;
      if (pathContext.numGeoLights > 0 && lastVertex.numLightSamples > 0) {
        // at the moment we only support uniform light selection
        // therefore we can evaluate the selection PDF directly
        const float lightSelectionPDF =
            lastVertex.numLightSamples / (float)pathContext.numLights;
        // convert pdf wrt. area to pdf wrt. solid angle
        const float cosd = dot(pathVertex.dg.Ng, ray.dir);
        // lastVertex.dg.P can be different from ray.org (previously sampled a
        // Dirac transmission), thus we cannot just use ray.t as distance
        const vec3f vdis = lastVertex.dg.P - pathVertex.dg.P;
        const float lePdf = pathVertex.dg.areaPDF * dot(vdis, vdis) / abs(cosd);
        misWeight = misHeuristic(
            pathContext, lastVertex.pdf_w, lightSelectionPDF * lePdf);
      }
      L = L + pathState.throughput * emission * misWeight;
    }
  }
  return L;
}

// Implementation /////////////////////////////////////////////////////////////

SYCL_EXTERNAL Light_SampleRes GeometryLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time,
    const uniform FeatureFlagsHandler &ffh)
{
  const GeometryLight *uniform self = (GeometryLight * uniform) super;
  const GeometricModel *uniform model = self->model;
  const Geometry *uniform geo = model->geom;
  Light_SampleRes res;
  res.weight = make_vec3f(0.0f);

  // choose primitive
  const Sample1D sample =
      Distribution1D_sample(self->numPrimitives, self->distribution, 0, s.x);
  const int32 primID = self->primIDs[sample.idx];

  Material *mat = GeometricModel_getMaterial(model, dg.renderer, primID);
  if (!mat)
    return res;

  const vec2f ns = make_vec2f(sample.frac, s.y); // reproject s.x
  // sample position on primitive
  // TODO maybe should already be done wrt. solid angle
  SampleAreaRes as;
  const Instance *uniform instance = self->super.instance;
  assert(instance);
  const uniform FeatureFlagsGeometry ffg = getFeatureFlagsGeometry(ffh);
  if (instance->motionBlur && (ffg & FFG_MOTION_BLUR)) {
    foreach_unique (utime in time) {
      const uniform affine3f xfm = Instance_getTransform(instance, utime);
      as = Geometry_dispatch_sampleArea(
          geo, primID, xfm, rcp(xfm), ns, time, ffh);
    }
  } else {
    as = Geometry_dispatch_sampleArea(
        geo, primID, instance->xfm, instance->rcp_xfm, ns, time, ffh);
  }
  // note that sample.pdf/primitives * sampleArea(worldspace).pdf == self->pdf

  // extant light vector from the hit point
  const vec3f dir = as.pos - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;

  // convert to pdf wrt. solid angle
  const float rcosd = rcp(abs(dot(as.normal, res.dir)));
  res.pdf = self->pdf * sqr(dist) * rcosd;

  res.dist = dist - max(as.epsilon, calcEpsilon(as.pos, dist)) * rcosd;

  DifferentialGeometry texDg;
  initDgFromTexCoord(texDg, as.st);
  texDg.P = as.pos;
  texDg.Ns = as.normal;
  texDg.Ng = as.normal;

  vec3f radiance;
  foreach_unique (m in mat)
    radiance = Material_dispatch_getEmission(m, texDg, ffh);

  res.weight = radiance * rcp(res.pdf);

  return res;
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform GeometryLight_sample_addr()
{
  return (void *uniform)GeometryLight_sample;
}

OSPRAY_END_ISPC_NAMESPACE
