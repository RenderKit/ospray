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

#include "math/random.ih"
#include "math/sampling.ih"
#include "render/Renderer.ih"
#include "render/simpleAO/SimpleAOMaterial.ih"

struct SimpleAO
{
  Renderer super;
  int samplesPerFrame;
  float aoRadius;
};

// Helper shading functions ///////////////////////////////////////////////////

inline vec3f getSurfaceColor(DifferentialGeometry &dg)
{
  uniform SimpleAOMaterial *mat = ((uniform SimpleAOMaterial *)dg.material);

  vec3f surfaceColor = make_vec3f(1.f);
  if (mat) {
    foreach_unique(m in mat)
    {
      surfaceColor = m->Kd;
      if (m->map_Kd) {
        vec4f Kd_from_map = get4f(m->map_Kd, dg);
        surfaceColor      = surfaceColor * make_vec3f(Kd_from_map);
      }
    }
  }

  // should be done in material:
  return surfaceColor * make_vec3f(dg.color);
}

inline float computeAO(const uniform SimpleAO *uniform self,
                       const FrameBuffer *uniform fb,
                       const World *uniform world,
                       const varying vec3i &sampleID,
                       const varying DifferentialGeometry &dg)
{
  const uniform int &sampleCnt = self->samplesPerFrame;
  const uniform int accumID    = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, 0x290374, (fb->size.x * sampleID.y) + sampleID.x);
  const vec2f rot = RandomTEA__getFloats(rng);

  int hits = 0;

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    const vec2f halton       = HaltonSequence_get2D(sampleCnt * accumID + i);
    const vec2f r            = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir       = localToWorld * local_ao_dir;

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, 0.0f, self->aoRadius);
    if (dot(ao_dir, dg.Ns) < 0.05f || isOccluded(world, ao_ray))
      hits++;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - hits / (float)sampleCnt;
}

inline vec4f computeShadedSurfaceColor(SimpleAO *uniform self,
                                       FrameBuffer *uniform fb,
                                       World *uniform world,
                                       const ScreenSample &sample)
{
  DifferentialGeometry dg;
  postIntersect(world,
                dg,
                sample.ray,
                DG_NG | DG_NS | DG_NORMALIZE | DG_FACEFORWARD | DG_MATERIALID |
                    DG_COLOR | DG_TEXCOORD);

  const vec3f surfaceColor = getSurfaceColor(dg);

  float intensity = 1.f;
  cif(self->samplesPerFrame > 0)
  {
    intensity = computeAO(self, fb, world, sample.sampleID, dg);
  }
  else
  {
    intensity = absf(dot(dg.Ns, sample.ray.dir));
  }

  return make_vec4f(surfaceColor * intensity, 1.f);
}

// Renderer callback //////////////////////////////////////////////////////////

void SimpleAO_renderSample(Renderer *uniform _self,
                           FrameBuffer *uniform fb,
                           World *uniform world,
                           void *uniform perFrameData,
                           varying ScreenSample &sample)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  traceRay(world, sample.ray);
  sample.z = sample.ray.t;

  if (noHit(sample.ray)) {
    sample.rgb   = make_vec3f(self->super.bgColor);
    sample.alpha = self->super.bgColor.w;
  } else {
    const vec4f color = computeShadedSurfaceColor(self, fb, world, sample);

    sample.rgb   = make_vec3f(color);
    sample.alpha = color.w;
  }
}

// SimpleAO C++ interface /////////////////////////////////////////////////////

export void *uniform SimpleAO_create(void *uniform cppE)
{
  uniform SimpleAO *uniform self = uniform new uniform SimpleAO;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = SimpleAO_renderSample;
  return self;
}

export void SimpleAO_set(void *uniform _self,
                         uniform int samplesPerFrame,
                         uniform float aoRadius)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  self->samplesPerFrame = samplesPerFrame;
  self->aoRadius        = aoRadius;
}
