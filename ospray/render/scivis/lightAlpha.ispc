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

#include "SciVisRenderer.ih"
#include "fb/FrameBuffer.ih"
#include "render/util.ih"
#include "texture/Texture2D.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "math/LinearSpace.ih"

#include "SciVisMaterial.ih"


// Lighting functions //
float lightAlpha(const uniform SciVisRenderer *uniform self,
                 Ray &ray, uniform Model *uniform model,
                 const float weight,
                 const varying float &rayOffset,
                 const varying vec3i &sampleID,
                 const uniform float quality)
{
  float alpha = 1.f;
  const float org_t_max = ray.t;
  uniform int remaining_depth = self->super.maxDepth;

  while (1) {
    traceRay(model,ray);

    if (ray.geomID >= 0) { // surfaces
      DifferentialGeometry dg;
      postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD | DG_COLOR);

      uniform SciVisMaterial *scivisMaterial =
        (uniform SciVisMaterial *)dg.material;

      float material_opacity = 1.f;

      if(scivisMaterial == NULL) {
        material_opacity = dg.color.w;
      } else {
        foreach_unique( mat in scivisMaterial ) {
          material_opacity = mat->d * get1f(mat->map_d, dg, 1.f);
          if (hasAlpha(mat->map_Kd)) {
            vec4f Kd_from_map = get4f(mat->map_Kd,dg);
            material_opacity *= Kd_from_map.w;
          }
        }
      }

      alpha = alpha * (1.f - material_opacity);
      if (alpha * weight < ALPHA_THRESHOLD)
        return 0.f;
    }

    // volumes
    // Get first intersected volume for each ray and set the ray bounds.
    float org_t = ray.t;
    float org_t0 = ray.t0;
    Volume *volume = SciVisRenderer_intersectVolumes(self, ray, rayOffset);
    float tBegin = max(org_t0, ray.t0);
    float tEnd = min(org_t, ray.t + self->volumeEpsilon);

    if (volume && tBegin < tEnd) {
      // print("found volume intersection!\n");
      // Provide ray offset for use with isosurface geometries (this value
      // ignored elsewhere).
      ray.time = -rayOffset * volume->samplingStep;
      float material_opacity = 1.f;
      foreach_unique (v in volume) {
        vec4f volumeColor = SciVisRenderer_computeVolumeInterval(self, v, ray, tBegin, tEnd, 0.98f, true, rayOffset, sampleID, quality);
        material_opacity = volumeColor.w;
      }

      alpha = alpha * (1.f - material_opacity);

      if (alpha * weight < ALPHA_THRESHOLD)
        return 0.f;
    }


    if (!volume && ray.geomID < 0)
      return alpha;
    if (--remaining_depth <= 0)
      return alpha;

    ray.t0 = org_t + self->super.epsilon;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

