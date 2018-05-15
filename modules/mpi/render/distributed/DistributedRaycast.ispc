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

//ospray
#include "common/Model.ih"
#include "render/Renderer.ih"
#include "render/util.ih"
#include "math/random.ih"
#include "math/sampling.ih"

struct DistributedRaycastRenderer
{
  uniform Renderer super;
  // TODO: For now it's sufficient just to know which regions
  // we own and their bounds, and which regions 'others' own and
  // their bounds. With that info we can properly setup the information
  // about the total # of tiles to expect for each image tile.
  uniform box3f *uniform myRegions;
  uniform int numMyRegions;
  uniform box3f *uniform othersRegions;
  uniform int numOthersRegions;
  uniform box3f *uniform ghostRegions;
  uniform int aoSamples;
};

struct RegionInfo
{
  uniform int currentRegion;
  uniform bool *uniform regionVisible;
};

void DistributedRaycastRenderer_testRegions(uniform DistributedRaycastRenderer *uniform self,
                                            uniform RegionInfo *uniform regionInfo,
                                            const varying ScreenSample &sample)
{
  for (uniform int i = 0; i < self->numMyRegions; ++i) {
    float t0, t1;
    intersectBox(sample.ray, self->myRegions[i], t0, t1);
    if (t0 < t1 && t0 >= sample.ray.t0 && t0 <= sample.ray.t) {
      regionInfo->regionVisible[i] = true;
    }
  }
  for (uniform int i = 0; i < self->numOthersRegions; ++i) {
    float t0, t1;
    intersectBox(sample.ray, self->othersRegions[i], t0, t1);
    if (t0 < t1 && t0 >= sample.ray.t0 && t0 <= sample.ray.t) {
      regionInfo->regionVisible[self->numMyRegions + i] = true;
    }
  }
}

// TODO: The main scivis renderer does this in a really strange way
Volume* DRR_intersectVolumes(uniform DistributedRaycastRenderer *uniform self,
                             varying Ray &ray,
                             const float regionEnter,
                             const float regionExit,
                             const float rayOffset)
{
  Volume *volume = NULL;
  Ray volumeRay = ray;
  vec2f interval = make_vec2f(regionExit);

  for (uniform int32 i = 0; i < self->super.model->volumeCount; ++i) {
    Volume *uniform v = self->super.model->volumes[i];
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
                                  const varying vec3i &sampleID,
                                  const varying DifferentialGeometry &dg,
                                  const varying vec3f &shadingNormal,
                                  const uniform box3f *uniform ghostRegion)
{
  const int accumID = sampleID.z;
  const int ix = sampleID.x;
  const int iy = sampleID.y;

  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, 0x290374, (self->super.fb->size.x * iy) + ix);
  const vec2f rot = RandomTEA__getFloats(rng);

  float occlusion = 0.f;
  const linear3f localToWorld = frame(shadingNormal);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f halton = HaltonSequence_get2D(accumID * self->aoSamples + i);
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f localAoDir = cosineSampleHemisphere(r);
    const vec3f aoDir = normalize(localToWorld * localAoDir);

    // Check if the ray goes below the surface
    if (dot(aoDir, shadingNormal) < 0.05f) { 
      occlusion += 1.f;
      continue;
    }

    Ray aoRay;
    setRay(aoRay, dg.P + (self->super.epsilon * shadingNormal), aoDir,
           self->super.epsilon, 1e20f);
    if (ghostRegion) {
      intersectBox(aoRay, *ghostRegion, aoRay.t0, aoRay.t);
    }
    // TODO: Clip with the corresponding ghostRegion, or region if no ghost
    // regions provided
    if (isOccluded(self->super.model, aoRay)) {
      occlusion += 1.f;
    }
  }
  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir) cancel
  return 1.0f - occlusion / self->aoSamples;
}

void DistributedRaycastRenderer_renderSample(uniform Renderer *uniform _self,
                                             void *uniform perFrameData,
                                             varying ScreenSample &sample)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;

  uniform RegionInfo *uniform regionInfo = (uniform RegionInfo *uniform)perFrameData;
  if (self->myRegions && regionInfo && regionInfo->currentRegion == 0) {
    DistributedRaycastRenderer_testRegions(self, regionInfo, sample);
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

  // Intersect with current region for this node's local data
  if (self->myRegions && regionInfo) {
    intersectBox(sample.ray, self->myRegions[regionInfo->currentRegion],
                 sample.ray.t0, sample.ray.t);
  }
  const float regionEnter = sample.ray.t0;
  const float regionExit = sample.ray.t;

  uniform box3f *uniform ghostRegion = NULL;
  if (self->ghostRegions && regionInfo) {
    ghostRegion = &self->ghostRegions[regionInfo->currentRegion];
  }

  Ray volRay = sample.ray;
  Ray geomRay = sample.ray;
  Volume *volume = DRR_intersectVolumes(self, volRay, regionEnter,
                                        regionExit, rayOffset);

  traceRay(self->super.model, geomRay);
  sample.z = min(geomRay.t, volRay.t);

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
      const uniform float volumeEpsilon = 0.0001;
      // See if we exited the current volume
      if (volRay.t0 >= volRay.t) {
        volRay.t0 = volRay.t + volumeEpsilon;
        volRay.t = regionExit;
        Volume *volume = DRR_intersectVolumes(self, volRay, regionEnter,
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
      dg.color = make_vec4f(0.f);
      postIntersect(self->super.model, dg, geomRay,
          DG_COLOR | DG_MATERIALID | DG_NG | DG_NS);
      const vec3f matColor = make_vec3f(dg.color);
      const vec3f specColor = make_vec3f(0.6);
      const vec3f viewDir = normalize(negate(geomRay.dir));
      // TODO: read the light params?
      const vec3f lightDir = normalize(make_vec3f(1.0));
      const vec3f dgNormal = normalize(dg.Ns);
      // Hard-coded Blinn-Phong. TODO: Materials API support
      vec3f geomColor = matColor * make_vec3f(0.1);
      if (dot(lightDir, dgNormal) > 0.0) {
        geomColor = geomColor + matColor * dot(lightDir, dgNormal)
          + specColor * pow(dot(dgNormal, normalize(viewDir + lightDir)), 20);
      }
      float occlusion = 1.0;
      if (self->aoSamples > 0) {
        occlusion = DRR_computeAmbientOcclusion(self, sample.sampleID,
                                                dg, dgNormal, ghostRegion);
      }
      currentContribution = make_vec4f(geomColor * occlusion, dg.color.w);

      // Find the next geometry hit by the ray, if it wasn't opaque
      if (dg.color.w < 0.99) {
        geomRay.t0 = geomRay.t + self->super.epsilon;
        geomRay.t = regionExit;
        geomRay.primID = -1;
        geomRay.geomID = -1;
        geomRay.instID = -1;
        traceRay(self->super.model, geomRay);
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
  self->myRegions = NULL;
  self->numMyRegions = 0;
  self->othersRegions = NULL;
  self->numOthersRegions = 0;
  self->aoSamples = 0;
  self->ghostRegions = NULL;

  return self;
}

export void DistributedRaycastRenderer_setRegions(void *uniform _self,
                                                    uniform box3f *uniform myRegions,
                                                    uniform int numMyRegions,
                                                    uniform box3f *uniform othersRegions,
                                                    uniform int numOthersRegions,
                                                    uniform int aoSamples,
                                                    uniform box3f *uniform ghostRegions)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;
  self->myRegions = myRegions;
  self->numMyRegions = numMyRegions;
  self->othersRegions = othersRegions;
  self->numOthersRegions = numOthersRegions;
  self->aoSamples = aoSamples;
  self->ghostRegions = ghostRegions;
}

