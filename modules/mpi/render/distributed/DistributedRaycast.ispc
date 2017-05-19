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

//ospray
#include "common/Model.ih"
#include "render/Renderer.ih"
#include "render/util.ih"

struct DistributedRaycastRenderer
{
  uniform Renderer super;
  // TODO: For now it's sufficient just to know which clip boxes
  // we own and their bounds, and which clip boxes 'others' own and
  // their bounds. With that info we can properly setup the information
  // about the total # of tiles to expect for each image tile.
  uniform box3f *uniform myClipBoxes;
  uniform int numMyClipBoxes;
  uniform box3f *uniform othersClipBoxes;
  uniform int numOthersClipBoxes;
};

struct ClipBoxInfo
{
  uniform int currentBox;
  uniform bool *uniform boxVisible;
};

void DistributedRaycastRenderer_testBoxes(uniform DistributedRaycastRenderer *uniform self,
                                          uniform ClipBoxInfo *uniform boxInfo,
                                          const varying ScreenSample &sample)
{
  for (uniform int i = 0; i < self->numMyClipBoxes; ++i) {
    float t0, t1;
    intersectBox(sample.ray, self->myClipBoxes[i], t0, t1);
    if (t0 < t1 && t0 >= sample.ray.t0 && t0 <= sample.ray.t) {
      boxInfo->boxVisible[i] = true;
    }
  }
  for (uniform int i = 0; i < self->numOthersClipBoxes; ++i) {
    float t0, t1;
    intersectBox(sample.ray, self->othersClipBoxes[i], t0, t1);
    if (t0 < t1 && t0 >= sample.ray.t0 && t0 <= sample.ray.t) {
      boxInfo->boxVisible[self->numMyClipBoxes + i] = true;
    }
  }
}

void DistributedRaycastRenderer_renderSample(uniform Renderer *uniform _self,
                                             void *uniform perFrameData,
                                             varying ScreenSample &sample)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;

  uniform ClipBoxInfo *uniform boxInfo = (uniform ClipBoxInfo *uniform)perFrameData;
  if (boxInfo && boxInfo->currentBox == 0) {
    DistributedRaycastRenderer_testBoxes(self, boxInfo, sample);
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

  // Intersect with the clipping box for this node's local data, if we have one
  if (self->myClipBoxes && boxInfo) {
    intersectBox(sample.ray, self->myClipBoxes[boxInfo->currentBox], sample.ray.t0, sample.ray.t);
  }

  traceRay(self->super.model, sample.ray);
  sample.z     = sample.ray.t;

  if (sample.ray.geomID < 0) {
    // The owner sends the background color, so we composite with a transparent
    // black instead of the scene's bgcolor
    sample.rgb = make_vec3f(0.0f);
    sample.alpha = 0.f;
  } else {
    // Spheres are cheap to shade so it's fine we do it early, potentially tossing
    // the result if the volume is opaque before the hit. This also assumes that
    // the spheres are opaque.
    DifferentialGeometry dg;
    dg.color = make_vec4f(0.f);
    postIntersect(self->super.model, dg, sample.ray,
                  DG_COLOR | DG_MATERIALID | DG_NG | DG_NS);
    sample.rgb = make_vec3f(dg.color) *
                 abs(dot(normalize(sample.ray.dir), normalize(dg.Ns)));
    sample.alpha = 1.f;
  }

  vec4f volumeColor = make_vec4f(0.f);
  // TODO: Support for more than one volume (put them in the Embree BVH?)
  if (self->super.model->volumeCount > 0) {
    // See if we hit the volume bounds
    Ray ray = sample.ray;
    float t0, t1;
    Volume *uniform volume = self->super.model->volumes[0];
    intersectBox(ray, volume->boundingBox, t0, t1);
    if (t0 < t1 && t0 >= ray.t0 && t0 <= ray.t) {
      // Sample offset placement correction, like in the data-parallel
      // raycast renderer. We must offset and step as if we're sampling a continuous
      // volume on a single node.
      float dt = volume->samplingStep * rcpf(volume->samplingRate);
      int i0 = (int)(t0 / dt);
      ray.t0 = (i0 + rayOffset)*dt;
      if (ray.t0 < t0) {
        ray.t0 += dt;
      }
      ray.t = min(t1, sample.ray.t);

      // Now raymarch the volume
      while (ray.t0 < ray.t && volumeColor.w < 1.0) {
        const vec3f coordinates = ray.org + ray.t0 * ray.dir;
        const float sample = volume->computeSample(volume, coordinates);

        TransferFunction *uniform tfcn = volume->transferFunction;
        // Look up the color associated with the volume sample.
        const vec3f sampleColor = tfcn->getColorForValue(tfcn, sample);
        const float opacity = tfcn->getOpacityForValue(tfcn, sample);

        // Set the color contribution for this sample only (do not accumulate).
        const vec4f color = clamp(opacity / volume->samplingRate)
          * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

        volumeColor = volumeColor + (1.f - volumeColor.w) * color;

        // Advance the ray
        volume->intersect(volume, ray);
      }
      volumeColor.w = clamp(volumeColor.w);
      if (sample.ray.geomID < 0) {
        sample.z = t1;
      }
    }
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
  self->myClipBoxes = NULL;
  self->othersClipBoxes = NULL;

  return self;
}

export void DistributedRaycastRenderer_setClipBoxes(void *uniform _self,
                                                    uniform box3f *uniform myClipBoxes,
                                                    uniform int numMyClipBoxes,
                                                    uniform box3f *uniform othersClipBoxes,
                                                    uniform int numOthersClipBoxes)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;
  self->myClipBoxes = myClipBoxes;
  self->numMyClipBoxes = numMyClipBoxes;
  self->othersClipBoxes = othersClipBoxes;
  self->numOthersClipBoxes = numOthersClipBoxes;
}

