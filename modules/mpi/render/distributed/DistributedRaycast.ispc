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

//ospray
#include "common/Model.ih"
#include "render/Renderer.ih"
#include "render/util.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "lights/Light.ih"
#include "render/scivis/SciVisMaterial.ih"
#include "render/scivis/surfaceShading.ih"

struct DistributedRegion
{
  uniform box3f bounds;
  uniform int id;
};

struct DistributedRaycastRenderer
{
  uniform Renderer super;
  // The list of all regions in the distributed scene, sorted by ID
  uniform DistributedRegion *uniform regions;
  uniform int numRegions;
  uniform int aoSamples;
  uniform bool oneSidedLighting;
  uniform bool shadowsEnabled;
  uniform vec3f ambientLight;
  uniform uint32 numLights;
  const uniform Light *uniform *uniform lights;
  uniform Model *uniform *uniform myRegions;
  uniform Model *uniform *uniform ghostRegions;
};

struct RegionInfo
{
  uniform int currentRegion;
  uniform bool computeVisibility;
  uniform bool *uniform regionVisible;
};

uniform bool isempty(uniform box3f &box) {
  return box.upper.x < box.lower.x
    || box.upper.y < box.lower.y
    || box.upper.z < box.lower.z;
}

void DistributedRaycastRenderer_testRegions(uniform DistributedRaycastRenderer *uniform self,
                                            uniform RegionInfo *uniform regionInfo,
                                            const varying ScreenSample &sample)
{
  for (uniform int i = 0; i < self->numRegions; ++i) {
    if (!regionInfo->regionVisible[i] && !isempty(self->regions[i].bounds)) {
      float t0, t1;
      intersectBox(sample.ray, self->regions[i].bounds, t0, t1);
      if (t0 < t1 && t0 >= sample.ray.t0 && t0 <= sample.ray.t) {
        regionInfo->regionVisible[i] = true;
      }
    }
  }
}

// TODO: The main scivis renderer does this in a really strange way
Volume* DRR_intersectVolumes(uniform Model *uniform model,
                             varying Ray &ray,
                             const float regionEnter,
                             const float regionExit,
                             const float rayOffset)
{
  Volume *volume = NULL;
  Ray volumeRay = ray;
  vec2f interval = make_vec2f(regionExit);

  for (uniform int32 i = 0; i < model->volumeCount; ++i) {
    Volume *uniform v = model->volumes[i];
    float t0, t1;
    intersectBox(volumeRay, v->boundingBox, t0, t1);

    // Clip against volume clipping box (if specified).
    if (ne(v->volumeClippingBox.lower,
           v->volumeClippingBox.upper)) {
      float tClip0, tClip1;
      intersectBox(ray, v->volumeClippingBox, tClip0, tClip1);

      t0 = max(t0, tClip0);
      t1 = min(t1, tClip1);
    }

    // And clip against the region interval
    t0 = max(t0, regionEnter);
    t1 = min(t1, regionExit);

    if (t0 < t1 && t0 < volumeRay.t) {
      interval.x = t0;
      interval.y = t1;
      volumeRay.t = t0;
      volume = v;
    }
  }

  volumeRay.t0 = interval.x;
  volumeRay.t = interval.y;

  if (volume) {
    // Sample offset placement correction, like in the data-parallel
    // raycast renderer. We must offset and step as if we're sampling a continuous
    // volume on a single node.
    float dt = volume->samplingStep * rcpf(volume->samplingRate);
    float t0 = volumeRay.t0;
    int i0 = (int)(volumeRay.t0 / dt);
    volumeRay.t0 = (i0 + rayOffset)*dt;
    if (volumeRay.t0 < t0) {
      volumeRay.t0 += dt;
    }
    volumeRay.t = min(volumeRay.t, regionExit);

  }
  // Update the user provided ray
  ray = volumeRay;
  return volume;
}

vec4f DRR_integrateVolumeSegment(uniform DistributedRaycastRenderer *uniform self,
                                 uniform Volume *uniform volume,
                                 const varying Ray &segment)
{
  vec4f volumeColor = make_vec4f(0.0);
  Ray ray = segment;
  while (ray.t0 < ray.t && volumeColor.w < 1.0) {
    const vec3f coordinates = ray.org + ray.t0 * ray.dir;

    const float sample = volume->sample(volume, coordinates);

    uniform TransferFunction *uniform tfcn = volume->transferFunction;
    // Look up the color associated with the volume sample.
    const vec3f sampleColor = tfcn->getColorForValue(tfcn, sample);
    const float opacity = tfcn->getOpacityForValue(tfcn, sample);

    // Set the color contribution for this sample only (do not accumulate).
    const vec4f color = clamp(opacity / volume->samplingRate)
      * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

    // Advance the ray
    volume->stepRay(volume, ray, volume->samplingRate);
    volumeColor = volumeColor + (1.f - volumeColor.w) * color;
  }
  volumeColor.w = clamp(volumeColor.w);
  return volumeColor;
}

float DRR_computeAmbientOcclusion(uniform DistributedRaycastRenderer *uniform self,
                                  uniform Model *uniform model,
                                  uniform Model *uniform ghostModel,
                                  const varying vec3i &sampleID,
                                  const varying DifferentialGeometry &dg,
                                  const varying SciVisShadingInfo &info)
{
  const int accumID = sampleID.z;
  const int ix = sampleID.x;
  const int iy = sampleID.y;

  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, 0x290374, (self->super.fb->size.x * iy) + ix);
  const vec2f rot = RandomTEA__getFloats(rng);

  float occlusion = 0.f;
  const linear3f localToWorld = frame(info.shadingNormal);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f halton = HaltonSequence_get2D(accumID * self->aoSamples + i);
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f localAoDir = cosineSampleHemisphere(r);
    const vec3f aoDir = normalize(localToWorld * localAoDir);

    // Check if the ray goes below the surface
    if (dot(aoDir, info.shadingNormal) < 0.05f) { 
      occlusion += 1.f;
      continue;
    }

    Ray aoRay;
    setRay(aoRay, dg.P, aoDir);

    if (isOccluded(model, aoRay)
        || (ghostModel && isOccluded(ghostModel, aoRay)))
    {
      occlusion += 1.f;
    }
  }
  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir) cancel
  return 1.0f - occlusion / self->aoSamples;
}

vec3f DRR_integrateOverLights(const uniform DistributedRaycastRenderer *uniform self,
                              uniform Model *uniform model,
                              uniform Model *uniform ghostModel,
                              const varying Ray &ray,
                              const varying DifferentialGeometry &dg,
                              const varying SciVisShadingInfo &info)
{
  vec3f color = make_vec3f(info.Kd * self->ambientLight);

  // Calculate shading for all lights
  for (uniform int i = 0; self->lights && i < self->numLights; i++) {
    const uniform Light *uniform l = self->lights[i];
    const vec2f s = make_vec2f(0.0f); // sample center of area lights
    const Light_SampleRes light = l->sample(l, dg, s);

    // any potential contribution?
    if (reduce_max(light.weight) > 0.f) {
      float cosNL = dot(light.dir, info.shadingNormal);
      if (cosNL < 0.0f) {
        if (self->oneSidedLighting)
          continue;
        cosNL = abs(cosNL);
      }

      const vec3f H = normalize(light.dir - ray.dir);
      const float cosNH = dot(H, info.shadingNormal);
      const vec3f specular = info.Ks * powf(cosNH, info.Ns);
      const vec3f diffuse = info.Kd * cosNL;
      const vec3f light_contrib =
        info.local_opacity * (diffuse + specular) * light.weight;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > self->super.minContribution) {
          vec3f P = dg.P;
          if (dot(dg.Ng, light.dir) < 0.0f) {
            P = P - (2.f * dg.epsilon) * dg.Ng;
          }

          Ray shadowRay;
          setRay(shadowRay, P, light.dir, 0.0f, light.dist, ray.time);
          if (!isOccluded(model, shadowRay)
              && !(ghostModel && isOccluded(ghostModel, shadowRay)))
          {
            color = color + light_contrib;
          }
        }
      } else {
        color = color + light_contrib;
      }
    }
  }
  return color;
}



void DistributedRaycastRenderer_renderSample(uniform Renderer *uniform _self,
                                             void *uniform perFrameData,
                                             varying ScreenSample &sample)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;

  uniform RegionInfo *uniform regionInfo = (uniform RegionInfo *uniform)perFrameData;

  // TODO WILL: It makes no sense that this fixes the visibility test issue.
  sample.ray.t = inf;
  sample.ray.t0 = 1e-6f;

  // TODO: This is basically a hack to do a tile-region visibility pre-pass
  // without needing to modify much code. Really we'd want to do a separate
  // pre-pass of some kind where we either project the regions to the screen,
  // or to handle jittering the rays test rays against them until we
  // find an intersection
  if (self->regions && regionInfo && regionInfo->computeVisibility) {
    DistributedRaycastRenderer_testRegions(self, regionInfo, sample);
    return;
  }

  // Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);
  if (rayOffset > 1.f) {
    rayOffset -= 1.f;
  }

  uniform Model *uniform model = NULL;
  uniform Model *uniform ghostModel = NULL;
  if (regionInfo) {
    model = self->myRegions[regionInfo->currentRegion];
    // TODO: Maybe allow people to set just one ghost region,
    // instead of requiring they always have equal number of regions
    // and ghost regions. May make some use cases easier, and reduce memory
    // overhead for those cases.
    if (self->ghostRegions) {
      ghostModel = self->ghostRegions[regionInfo->currentRegion];
    }
  } else {
    model = self->super.model;
  }

  // Intersect with current region for this node's local data
  if (self->regions && regionInfo) {
    intersectBox(sample.ray, model->bounds, sample.ray.t0, sample.ray.t);
  }
  const float regionEnter = sample.ray.t0;
  const float regionExit = sample.ray.t;

  Ray volRay = sample.ray;
  Ray geomRay = sample.ray;
  Volume *volume = DRR_intersectVolumes(model, volRay, regionEnter,
                                        regionExit, rayOffset);

  traceRay(model, geomRay);
  sample.z = min(geomRay.t, volRay.t0);

  // Loop through the hits, we should integrate the volume to the first
  // geometry hit point, if the hit of the geometry is inside the volume
  // (sample.ray.t), then shade the geometry, find the next
  // geometry intersection, integrate the volume along this new segment, then
  // shade the geometry and so on.
  vec4f color = make_vec4f(0.f);
  float firstHit;
  while ((firstHit = min(geomRay.t, volRay.t0)) < regionExit && color.w < 0.99) {
    vec4f currentContribution = make_vec4f(0);
    // Shade the current volume interval if it's before the next geometry
    if (firstHit == volRay.t0) {
      const uniform float volumeEpsilon = 0.001;
      // See if we exited the current volume
      if (volRay.t0 >= volRay.t) {
        volRay.t0 = volRay.t + volumeEpsilon;
        volRay.t = regionExit;
        Volume *volume = DRR_intersectVolumes(model, volRay, regionEnter,
                                              regionExit, rayOffset);
      } else {
        if (any(volume == NULL)) {
          print("ACCESSING NULL VOLUME!\n");
        }
        volRay.t = min(geomRay.t, volRay.t);
        foreach_unique(v in volume) {
          currentContribution = DRR_integrateVolumeSegment(self, v, volRay);
        }
        volRay.t0 = volRay.t + volumeEpsilon;
      }
    } else {
      // Shade the current geometry hit
      DifferentialGeometry dg;
      postIntersect(model, dg, geomRay,
                    DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_TANGENTS|
                    DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

      SciVisShadingInfo info;
      initShadingInfo(info);

      shadeMaterials(dg, info);
      info.local_opacity = info.d;

      vec3f geomColor = DRR_integrateOverLights(self, model, ghostModel,
                                                geomRay, dg, info);

      float occlusion = 1.0;
      if (self->aoSamples > 0) {
        occlusion = DRR_computeAmbientOcclusion(self,
                                                model,
                                                ghostModel,
                                                sample.sampleID,
                                                dg, info);
      }

      currentContribution = make_vec4f(geomColor * occlusion,
                                       info.local_opacity);

      geomRay.t0 = geomRay.t + dg.epsilon;
      geomRay.t = regionExit;
      geomRay.primID = -1;
      geomRay.geomID = -1;
      geomRay.instID = -1;
      // Find the next geometry hit by the ray, if we're not opaque
      if (color.w + (1.0 - color.w) * currentContribution.w < 0.99) {
        traceRay(model, geomRay);
      }
    }
    color = color + (1.0 - color.w) * currentContribution;
  }
  sample.rgb = make_vec3f(color);
  sample.alpha = color.w;
}

// Exported functions /////////////////////////////////////////////////////////

export void *uniform DistributedRaycastRenderer_create(void *uniform cppE) {
  uniform DistributedRaycastRenderer *uniform self =
    uniform new uniform DistributedRaycastRenderer;

  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = DistributedRaycastRenderer_renderSample;
  self->regions = NULL;
  self->numRegions = 0;
  self->aoSamples = 0;
  self->shadowsEnabled = false;
  self->ghostRegions = NULL;

  return self;
}

export void DistributedRaycastRenderer_set(void *uniform _self,
                                           void *uniform regions,
                                           const uniform int numRegions,
                                           const uniform int aoSamples,
                                           const uniform bool oneSidedLighting,
                                           const uniform bool shadowsEnabled,
                                           const uniform vec3f *uniform ambientLight,
                                           uniform uint32 numLights,
                                           void *uniform *uniform lights,
                                           void *uniform *uniform myRegions,
                                           void *uniform *uniform ghostRegions)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;
  // Correct the params the parent probably got wrong if we took
  // a list of OSPModels
  if (regions != NULL) {
    self->super.model = NULL;
  }

  self->regions = (uniform DistributedRegion *uniform)regions;
  self->numRegions = numRegions;
  self->aoSamples = aoSamples;
  self->oneSidedLighting = oneSidedLighting;
  self->shadowsEnabled = shadowsEnabled;
  self->ambientLight = *ambientLight;
  self->numLights = numLights;
  self->lights = (const uniform Light *uniform *uniform)lights;
  self->myRegions = (uniform Model *uniform *uniform)myRegions;
  self->ghostRegions = (uniform Model *uniform *uniform)ghostRegions;
}

