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
                             const varying ScreenSample &sample,
                             varying Ray &ray,
                             const float rayOffset)
{
  Volume *volume = NULL;
  Ray volumeRay = sample.ray;
  vec2f interval = make_vec2f(volumeRay.t0, volumeRay.t);

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

    if (t0 < t1 && t0 < volumeRay.t) {
      interval.x = max(t0, sample.ray.t0);
      interval.y = min(t1, sample.ray.t);
      volumeRay.t = t0;
      volume = v;
    }
  }
  if (volume) {
    volumeRay.t0 = interval.x;
    volumeRay.t = interval.y;

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
    volumeRay.t = min(volumeRay.t, sample.ray.t);

    // Update the user provided ray
    ray = volumeRay;
  }
  return volume;
}

vec4f DistributedRaycastRenderer_integrateVolumes(uniform DistributedRaycastRenderer *uniform self,
                                                  const varying ScreenSample &sample,
                                                  const float rayOffset)
{
  vec4f volumeColor = make_vec4f(0.0);
  // See if we hit the volume bounds
  Ray ray = sample.ray;
  Volume *volume = DRR_intersectVolumes(self, sample, ray, rayOffset);
  if (volume) {
    // Now raymarch the volume
    while (ray.t0 < ray.t && volumeColor.w < 1.0) {
      const vec3f coordinates = ray.org + ray.t0 * ray.dir;
      vec4f color = make_vec4f(0.0);

      foreach_unique (v in volume) {
        const float sample = v->sample(v, coordinates);

        TransferFunction *uniform tfcn = v->transferFunction;
        // Look up the color associated with the volume sample.
        const vec3f sampleColor = tfcn->getColorForValue(tfcn, sample);
        const float opacity = tfcn->getOpacityForValue(tfcn, sample);

        // Set the color contribution for this sample only (do not accumulate).
        color = clamp(opacity / v->samplingRate)
          * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

        // Advance the ray
        v->stepRay(v, ray, volume->samplingRate);
      }
      volumeColor = volumeColor + (1.f - volumeColor.w) * color;
    }
    volumeColor.w = clamp(volumeColor.w);

    ray.t = sample.ray.t;
    volume = DRR_intersectVolumes(self, sample, ray, rayOffset);
  }
  return volumeColor;
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
    intersectBox(sample.ray, self->myRegions[regionInfo->currentRegion], sample.ray.t0, sample.ray.t);
  }

  traceRay(self->super.model, sample.ray);
  sample.z = sample.ray.t;

  if (sample.ray.geomID < 0) {
    // The owner sends the background color, so we composite with a transparent
    // black instead of the scene's bgcolor
    sample.rgb = make_vec3f(0.0f);
    sample.alpha = 0.f;
  } else {
    // Spheres are cheap to shade so it's fine we do it early, potentially tossing
    // the result if the volume is opaque before the hit. This also assumes that
    // the spheres are opaque, since we treat the hit point as the end of the
    // ray, and send alpha to 1 for this sample.
    DifferentialGeometry dg;
    dg.color = make_vec4f(0.f);
    postIntersect(self->super.model, dg, sample.ray,
                  DG_COLOR | DG_MATERIALID | DG_NG | DG_NS);
    const vec3f matColor = make_vec3f(dg.color);
    const vec3f specColor = make_vec3f(0.6);
    const vec3f viewDir = normalize(negate(sample.ray.dir));
    // TODO: read the light params?
    const vec3f lightDir = normalize(make_vec3f(1.0));
    const vec3f dgNormal = normalize(dg.Ns);
    // Hard-coded Blinn-Phong. TODO: Materials API support
    sample.rgb = matColor * make_vec3f(0.05);
    if (dot(lightDir, dgNormal) > 0.0) {
      sample.rgb = sample.rgb + matColor * dot(lightDir, dgNormal)
                + specColor * pow(dot(dgNormal, normalize(viewDir + lightDir)), 20);
    }
    // TODO: Support transparent geometries
    sample.alpha = 1.f;
  }

  vec4f volumeColor = make_vec4f(0.f);
  // TODO: Support for more than one volume (put them in the Embree BVH?)
  if (self->super.model->volumeCount > 0) {
    volumeColor = DistributedRaycastRenderer_integrateVolumes(self, sample, rayOffset);
  }
  // Composite the geometry
  sample.rgb = make_vec3f(volumeColor.x, volumeColor.y, volumeColor.z)
    + (1.f - volumeColor.w) * sample.rgb;
  sample.alpha = volumeColor.w + (1.f - volumeColor.w) * sample.alpha;
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

  return self;
}

export void DistributedRaycastRenderer_setRegions(void *uniform _self,
                                                    uniform box3f *uniform myRegions,
                                                    uniform int numMyRegions,
                                                    uniform box3f *uniform othersRegions,
                                                    uniform int numOthersRegions)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;
  self->myRegions = myRegions;
  self->numMyRegions = numMyRegions;
  self->othersRegions = othersRegions;
  self->numOthersRegions = numOthersRegions;
}

