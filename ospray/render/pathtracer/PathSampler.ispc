// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "render/pathtracer/PathSampler.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/ShadowCatcher.ih"
#include "render/pathtracer/TransparentShadow.ih"
#include "render/pathtracer/GeometryLight.ih"
#include "render/pathtracer/VirtualLight.ih"
#include "render/pathtracer/NextEventEstimation.ih"

#include "render/pathtracer/bsdfs/BSDF.ih"
#include "render/pathtracer/bsdfs/MicrofacetAlbedoTables.ih"
#include "render/pathtracer/materials/Medium.ih"
#include "render/pathtracer/materials/Material.ih"
#include "render/pathtracer/volumes/VolumeSampler.ih"
#include "common/Intersect.ih"

inline void postIntersect(const PathContext& pathContext,
                          const PathState& pathState, 
                          PathVertex& pathVertex, Ray& ray)
{
  if (pathVertex.type == SURFACE)
  {
    postIntersect(pathContext.world,
        pathVertex.dg,
        ray,
        DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR
            | DG_TANGENTS);
  }
  if (pathVertex.type == VOLUME)
  {
      pathVertex.dg.Ng = make_vec3f(0.f);
      pathVertex.dg.Ns = make_vec3f(0.f);
      pathVertex.dg.dPds = make_vec3f(0.f); 
      pathVertex.dg.dPdt = make_vec3f(0.f);
      pathVertex.dg.st = make_vec2f(0);
      pathVertex.dg.color = make_vec4f(0.f);
      pathVertex.dg.primID = RTC_INVALID_GEOMETRY_ID;
      pathVertex.dg.areaPDF = 0.f;
      pathVertex.dg.epsilon = 0.f;
      pathVertex.dg.material = NULL;
  }
}

inline Scattering_SampleRes sampleDirection(const PathContext& pathContext, 
                                            const PathState& pathState, 
                                            PathVertex& pathVertex)
{
  const vec2f ss = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim);
  const float s  = LDSampler_getFloat (pathContext.ldSampler, pathState.sampleDim+2);
  Scattering_SampleRes fs;
  if (pathVertex.type == SURFACE)
  {
    vec3f Ns = pathVertex.dg.Ns;
    foreach_unique(f in pathVertex.bsdf) {
      if (f != NULL) {
        fs = f->sample(f, pathVertex.wo, ss, s);
        pathVertex.wi  = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
  if (pathVertex.type == VOLUME)
  {
    foreach_unique(v in pathVertex.volume) {
      if (v != NULL) {
        fs = HenyeyGreenstein_sample(v->anisotropy, pathVertex.wo, ss, s);
        pathVertex.wi  = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
  return fs;
}

void samplePath(const PathContext& pathContext,
                PathState& pathState, 
                Ray& ray, 
                ScreenSample& sample)
{
  PathVertex lastVertex;
  lastVertex.type = CAMERA;
  lastVertex.volume = NULL;
  lastVertex.pdf_w = inf;    // probability density of previous sampled BSDF, for MIS
  lastVertex.dg.P = ray.org; // P and N also used by light eval
  lastVertex.dg.epsilon = calcEpsilon(ray.org, 0.f);
  lastVertex.dg.Ns = ray.dir;
  lastVertex.dg.Ng = ray.dir;

  uniform ShadingContext ctx;
  ShadingContext_Constructor(&ctx);

  if (pathContext.context->shadowCatcher)
  {
    const Hit hit = intersectPlane(
        ray.org, ray.dir, pathContext.context->shadowCatcherPlane);
    if (hit.hit)
      pathState.shadowCatcherDist = hit.t;
  }
  do {
    if (pathState.shadowCatcherDist > ray.t0) // valid hit can hide other geometry
      ray.t = min(pathState.shadowCatcherDist, ray.t);

    traceRay(pathContext.world, ray);

    PathVertex pathVertex;
    pathVertex.pdf_w = inf;
    pathVertex.volume = NULL;
    if (noHit(ray)) {
      pathVertex.type = ENVIRONMENT;
    } else {
      pathVertex.type = SURFACE;
    }

    // record depth of primary rays
    if (pathState.depth == 0)
      sample.z = ray.t;

    if (shadowCatcher(pathContext, pathState, pathVertex, ray, sample))
      break;

    pathVertex.wo = neg(ray.dir);
    
    float extinctionCoefficient;
    float freePath = volumeSampleFreePath(pathState, pathContext.world, ray, 
      pathContext.ldSampler, pathContext.randomSampler, 
      &pathVertex.volume, extinctionCoefficient, pathVertex.albedo);
    if (freePath < inf)
    {
      pathVertex.type = VOLUME;
      pathVertex.dg.P = make_vec3f(ray.org + ray.dir * freePath);
      pathState.throughput = pathState.throughput * pathVertex.albedo;
    }

    // backplate handling
    if (pathVertex.type == ENVIRONMENT && pathState.straightPath) 
    {
      if ((bool)pathContext.context->backplate) {
        DifferentialGeometry lookup;
        initDgFromTexCoord(lookup, clamp2edge(pathContext.context->backplate, *pathContext.pixel));
        pathState.contribution = pathState.contribution + pathState.throughput * get3f(pathContext.context->backplate, lookup);
      }
      else
        sample.alpha = 1.0f - luminance(pathState.throughput);
    }    

    if ((!pathState.disableFWD || pathState.depth == 0) && pathVertex.type != VOLUME) {
      pathState.contribution = pathState.contribution + evaluateVirtualLights(pathContext, pathState, lastVertex, pathVertex, ray);
    }

    if (pathVertex.type == ENVIRONMENT)
      break;

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    // except if we have geometry lights (which we still need to evaluate for MIS)
    if (pathState.depth >= pathContext.context->super.maxDepth && pathContext.context->numGeoLights == 0)
      break;

    postIntersect(pathContext, pathState, pathVertex, ray);

    if (!pathState.disableFWD && pathVertex.type != VOLUME) {
      pathState.contribution = pathState.contribution + evaluateGeometryLights(pathState, lastVertex, pathVertex, ray);
    }

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    if (pathState.depth >= pathContext.context->super.maxDepth)
      break;

    // shade surface
    ShadingContext_Constructor(&ctx);
    if (pathVertex.type == SURFACE) {
      uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;
      foreach_unique(m in material)
        if (m != NULL)
          pathVertex.bsdf = m->getBSDF(m, &ctx, pathVertex.dg, ray, pathState.currentMedium);
      // terminate path when we don't have any BSDF
      if (!pathVertex.bsdf)
        break;
      pathVertex.albedo = pathVertex.bsdf->albedo;
    }

    // next event estimation
    if (!pathState.disableNEE && isSmooth(pathVertex)) {
      pathState.contribution = pathState.contribution + nextEventEstimation(pathContext, pathState, pathVertex);
    }

    Scattering_SampleRes fs = sampleDirection(pathContext, pathState, pathVertex);
    
    if (pathState.auxFree && (fs.type & SCATTERING_SMOOTH)) {
      updateAuxilliaryData(pathState, pathVertex, sample);
    }

    // terminate path when zero contribution from material
    if (reduce_max(fs.weight) <= 0.0f || fs.pdf <= PDF_CULLING)
      break;

    pathState.throughput = pathState.throughput * fs.weight;

#if ROULETTE==1
    // Russian roulette
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float rr = LDSampler_getFloat(pathContext.ldSampler, pathState.sampleDim+3);
      const float contProb = min(luminance(pathState.throughput), MAX_ROULETTE_CONT_PROB);
      if (rr > contProb)
        break;
      pathState.throughput = pathState.throughput * rcp(contProb);
      fs.pdf *= contProb;
    }
#endif

    // compute attenuation with Beer's law
    if (reduce_min(pathState.currentMedium.attenuation) < 0.f)
      pathState.throughput = pathState.throughput * expf(pathState.currentMedium.attenuation * ray.t);

    vec3f ray_org = pathVertex.dg.P;
    if (pathVertex.type == SURFACE)
    {
      // update currentMedium if we hit a medium interface
      // TODO: support nested dielectrics
      if (fs.type & SCATTERING_TRANSMISSION) {
        ray_org = ray_org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
        uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;
        foreach_unique(m in material) {
          if (m != NULL)
            m->selectNextMedium(m, pathVertex.dg, pathState.currentMedium);
        }
      }
    }

    // keep lastBsdfPdf and lastDg when there was a specular transmission
    // to better combine MIS with transparent shadows
    if (fs.type & ~SCATTERING_SPECULAR_TRANSMISSION || pathVertex.type == VOLUME) {
      lastVertex = pathVertex;
    }

    // continue the path
    pathState.straightPath &= eq(ray.dir, fs.wi);
    setRay(ray, ray_org, fs.wi, pathState.time);
    pathState.depth++;
    pathState.sampleDim += pathContext.numBounceSampleDims;
  } while (reduce_max(pathState.throughput) > pathContext.context->super.minContribution);

  sample.rgb = pathState.contribution;
  if (isnan(pathState.contribution.x) || 
      isnan(pathState.contribution.y) || 
      isnan(pathState.contribution.z))
  {
    sample.rgb = make_vec3f(0.f);
    sample.alpha = 1.0f;
  }
}
