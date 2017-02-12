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
  uniform int remaining_depth = self->maxDepth;

  while (1) {
    // Get first intersected volume for each ray and set the ray bounds.
    float org_t = ray.t;
    float org_t0 = ray.t0;
    Volume *volume = SciVisRenderer_intersectVolumes(self, ray, rayOffset);
    float volumeT = ray.t;
    float volumeT0 = ray.t0;
    ray.t = org_t;
    ray.t0 = org_t0;

    traceRay(model,ray);

    if (!(volumeT0 <= 0.f || volumeT0 <= ray.t))
      volume = NULL;

    if (!volume && ray.geomID < 0) return alpha;

    float material_opacity = 1.f;

    if (!volume)
      {
        DifferentialGeometry dg;
        postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD | DG_COLOR);

        uniform SciVisMaterial *scivisMaterial =
          (uniform SciVisMaterial *)dg.material;

        if(scivisMaterial == NULL) {
          material_opacity = dg.color.w;
        } else {
          foreach_unique( mat in scivisMaterial ) {
            material_opacity = mat->d * get1f(mat->map_d, dg.st, 1.f);
            if (mat->map_Kd) {
              vec4f Kd_from_map = get4f(mat->map_Kd,dg.st);
              material_opacity *= Kd_from_map.w;
            }
          }
        }
      }
    else
      {
        ray.t = org_t;
        ray.t0 = org_t0;
        // print("found volume intersection!\n");
        // Provide ray offset for use with isosurface geometries (this value
        // ignored elsewhere).
        ray.time = -rayOffset * volume->samplingStep;
        ray.t = volumeT;
        float tBegin = max(volumeT0,0.f);
        float tEnd = volumeT+self->volumeEpsilon;
        foreach_unique (v in volume)
          {
            vec4f volumeColor = SciVisRenderer_computeVolumeInterval(self, v, ray, tBegin, tEnd, 0.98f, 1, rayOffset, sampleID, quality);
            material_opacity = volumeColor.w;
          }  
      }

    alpha = alpha * (1.f - material_opacity);

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    if (--remaining_depth <= 0)
      return alpha;

    ray.t0 = ray.t + rayOffset;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

