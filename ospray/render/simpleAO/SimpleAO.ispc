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

#include "fb/FrameBuffer.ih"
#include "camera/Camera.ih"
#include "common/Ray.ih"
#include "render/util.ih"
#include "math/math.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "common/Model.ih"
#include "render/Renderer.ih"
#include "render/simpleAO/SimpleAOMaterial.ih"

struct SimpleAO
{
  uniform Renderer super;
  uniform int samplesPerFrame;
  uniform float aoRayLength;
};

inline void shade_ao(uniform SimpleAO *uniform self,
                     varying vec3f &color,
                     varying float &alpha,
                     const uniform int sampleCnt,
                     const uniform int accumID,
                     const Ray &ray,
                     const int32 pixel_x,
                     const int32 pixel_y)
{
  if (noHit(ray)) {
    color = make_vec3f(self->super.bgColor);
    alpha = self->super.bgColor.w;
    return;
  }

  DifferentialGeometry dg;
  postIntersect(self->super.model,dg,ray,
                DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD
                |DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

  uniform SimpleAOMaterial *mat = ((uniform SimpleAOMaterial*)dg.material);
  vec3f superColor = make_vec3f(1.f);
  if (mat) {
    foreach_unique(m in mat) {
      superColor = m->Kd;
      if (m->map_Kd) {
        vec4f Kd_from_map = get4f(m->map_Kd,dg);
        superColor = superColor * make_vec3f(Kd_from_map);
      }
    }
  }
  // should be done in material:
  superColor = superColor * make_vec3f(dg.color);

  // init TEA RNG //
  uniform FrameBuffer *uniform fb = self->super.fb;
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, 0x290374,(fb->size.x * pixel_y) + pixel_x);
  const vec2f rot = RandomTEA__getFloats(rng);

  int hits = 0;
  const vec3f N = dg.Ns;
  const linear3f localToWorld = frame(N);

  for (uniform int i = 0; i < sampleCnt; i++) {
    const vec2f halton = HaltonSequence_get2D(sampleCnt * accumID + i);
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, 0.0f, self->aoRayLength);
    if (dot(ao_dir, N) < 0.05f || isOccluded(self->super.model, ao_ray))
      hits++;
  }

  float diffuse = absf(dot(N,ray.dir));
  color = superColor * make_vec3f(diffuse * (1.0f - hits/(float)sampleCnt));
  alpha = 1.f;
  return;
}

void SimpleAO_renderSample(uniform Renderer *uniform _self,
                           void *uniform perFrameData,
                           varying ScreenSample &sample)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO *uniform)_self;

  traceRay(self->super.model, sample.ray);
  sample.z = sample.ray.t;

  const uniform int accumID =
      reduce_max(sample.sampleID.z) * self->samplesPerFrame;

  shade_ao(self,
           sample.rgb,
           sample.alpha,
           self->samplesPerFrame,
           accumID,
           sample.ray,
           sample.sampleID.x,
           sample.sampleID.y);
}


export void *uniform SimpleAO_create(void *uniform cppE)
{
  uniform SimpleAO *uniform self = uniform new uniform SimpleAO;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = SimpleAO_renderSample;
  return self;
}

export void SimpleAO_set(void *uniform _self,
                         uniform int samplesPerFrame,
                         uniform float aoRayLength)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO *uniform)_self;
  self->samplesPerFrame = samplesPerFrame;
  self->aoRayLength = aoRayLength;
}
