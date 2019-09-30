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

// ospray
#include "math/random.ih"
#include "camera/PerspectiveCamera.ih"
#include "fb/FrameBuffer.ih"
#include "render/Renderer.ih"
#include "render/util.ih"
#include "volume/VolumetricModel.ih"

#include "openvkl/openvkl.isph"

/*! \brief a simple, primary-ray-only ray cast render that

  This simple renderer shoots only a single primary ray and does some
  simple shading, mostly for debugging purposes such as visualizing
  primitive ID, geometry ID, shading normals, eyelight shading, etc */
struct RaycastRenderer
{
  /*! the base "Renderer" we are derived from */
  Renderer super;
};

/*! a simple test-frame renderer that doesn't even trace a ray, just
  returns a well-defined test frame (mostly useful for debugging
  whether frame buffers are properly set up etcpp */
void RaycastRenderer_renderSample_testFrame(Renderer *uniform _self,
                                            FrameBuffer *uniform fb,
                                            World *uniform world,
                                            void *uniform perFrameData,
                                            varying ScreenSample &sample)
{
  sample.rgb.x = ((sample.sampleID.x) % 256) / 255.f;
  sample.rgb.y = ((sample.sampleID.y) % 256) / 255.f;
  sample.rgb.z =
      ((sample.sampleID.x + sample.sampleID.y + sample.sampleID.z) % 256) /
      255.f;
  sample.alpha = 1.f;
  sample.z     = 1.f;
}

/*! a simple test-frame renderer that doesn't even trace a ray, just
  returns the absolute of the ray direction */
void RaycastRenderer_renderSample_rayDir(Renderer *uniform _self,
                                         FrameBuffer *uniform fb,
                                         World *uniform world,
                                         void *uniform perFrameData,
                                         varying ScreenSample &sample)
{
  sample.rgb   = absf(sample.ray.dir);
  sample.alpha = 1.f;
  sample.z     = 1.f;
}

void RaycastRenderer_renderSample_eyeLight(Renderer *uniform _self,
                                           FrameBuffer *uniform fb,
                                           World *uniform world,
                                           void *uniform perFrameData,
                                           varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    const float c = dot(normalize(sample.ray.Ng), sample.ray.dir);
    sample.rgb    = make_vec3f(.2f + .8f * abs(c));
  }
}

void RaycastRenderer_renderSample_Ng(Renderer *uniform _self,
                                     FrameBuffer *uniform fb,
                                     World *uniform world,
                                     void *uniform perFrameData,
                                     varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    postIntersect(world, dg, sample.ray, DG_NORMALIZE | DG_NG);
    sample.rgb = absf(dg.Ng);
  }
}

void RaycastRenderer_renderSample_Ns(Renderer *uniform _self,
                                     FrameBuffer *uniform fb,
                                     World *uniform world,
                                     void *uniform perFrameData,
                                     varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    postIntersect(world, dg, sample.ray, DG_NORMALIZE | DG_NS);
    sample.rgb = absf(dg.Ns);
  }
}

void RaycastRenderer_renderSample_dPds(Renderer *uniform _self,
                                       FrameBuffer *uniform fb,
                                       World *uniform world,
                                       void *uniform perFrameData,
                                       varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    postIntersect(world, dg, sample.ray, DG_TANGENTS);
    sample.rgb = normalize(dg.dPds);
    if (sample.rgb.x < 0.f)
      sample.rgb.x = sample.rgb.x * -0.3f;
    if (sample.rgb.y < 0.f)
      sample.rgb.y = sample.rgb.y * -0.3f;
    if (sample.rgb.z < 0.f)
      sample.rgb.z = sample.rgb.z * -0.3f;
  }
}

void RaycastRenderer_renderSample_dPdt(Renderer *uniform _self,
                                       FrameBuffer *uniform fb,
                                       World *uniform world,
                                       void *uniform perFrameData,
                                       varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    postIntersect(world, dg, sample.ray, DG_TANGENTS);
    sample.rgb = normalize(dg.dPdt);
    if (sample.rgb.x < 0.f)
      sample.rgb.x = sample.rgb.x * -0.3f;
    if (sample.rgb.y < 0.f)
      sample.rgb.y = sample.rgb.y * -0.3f;
    if (sample.rgb.z < 0.f)
      sample.rgb.z = sample.rgb.z * -0.3f;
  }
}

void RaycastRenderer_renderSample_eyeLight_vertexColor(
    Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    dg.color = make_vec4f(0.f);
    postIntersect(world, dg, sample.ray, DG_COLOR | DG_NG | DG_NS);
    sample.rgb = make_vec3f(dg.color) *
                 abs(dot(normalize(sample.ray.dir), normalize(dg.Ns)));
  }
}

void RaycastRenderer_renderSample_eyeLight_primID(Renderer *uniform _self,
                                                  FrameBuffer *uniform fb,
                                                  World *uniform world,
                                                  void *uniform perFrameData,
                                                  varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    const float c = dot(normalize(sample.ray.Ng), sample.ray.dir);
    sample.rgb =
        make_vec3f(.3f + .8f * abs(c)) * make_random_color(sample.ray.primID);
  }
}

void RaycastRenderer_renderSample_eyeLight_instID(Renderer *uniform _self,
                                                  FrameBuffer *uniform fb,
                                                  World *uniform world,
                                                  void *uniform perFrameData,
                                                  varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    const float c = dot(normalize(sample.ray.Ng), sample.ray.dir);
    sample.rgb =
        make_vec3f(.3f + .8f * abs(c)) * make_random_color(sample.ray.instID);
  }
}

void RaycastRenderer_renderSample_eyeLight_geomID(Renderer *uniform _self,
                                                  FrameBuffer *uniform fb,
                                                  World *uniform world,
                                                  void *uniform perFrameData,
                                                  varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    const float c = dot(normalize(sample.ray.Ng), sample.ray.dir);
    sample.rgb =
        make_vec3f(.3f + .8f * abs(c)) * make_random_color(sample.ray.geomID);
  }
}

void RaycastRenderer_renderSample_backfacing_Ng(Renderer *uniform _self,
                                                FrameBuffer *uniform fb,
                                                World *uniform world,
                                                void *uniform perFrameData,
                                                varying ScreenSample &sample)
{
  uniform RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    const float c = dot(normalize(sample.ray.Ng), sample.ray.dir);
    sample.rgb    = make_vec3f(.2f + .8f * abs(c));
    if (c > 0.f)
      sample.rgb.y = 0.f;
  }
}

void RaycastRenderer_renderSample_backfacing_Ns(Renderer *uniform _self,
                                                FrameBuffer *uniform fb,
                                                World *uniform world,
                                                void *uniform perFrameData,
                                                varying ScreenSample &sample)
{
  uniform RaycastRenderer *uniform self =
      (uniform RaycastRenderer * uniform) _self;

  traceRay(world, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    postIntersect(world, dg, sample.ray, DG_NORMALIZE | DG_NS);
    const float c = dot(dg.Ns, sample.ray.dir);
    sample.rgb    = make_vec3f(.2f + .8f * abs(c));
    if (c > 0.f)
      sample.rgb.y = 0.f;
  }
}

void RaycastRenderer_renderSample_volume(Renderer *uniform _self,
                                         FrameBuffer *uniform fb,
                                         World *uniform world,
                                         void *uniform perFrameData,
                                         varying ScreenSample &sample)
{
  RaycastRenderer *uniform self = (RaycastRenderer * uniform) _self;
  
  LDSampler ldSamplerObj;
  varying LDSampler* uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler, sample.sampleID.x + fb->size.x * sample.sampleID.y, sample.sampleID.z);

  vec3f &color = sample.rgb;
  float &alpha = sample.alpha;

  color = make_vec3f(self->super.bgColor);
  alpha = self->super.bgColor.w;

  VolumeInterval vInterval;
  traceVolumeRay(world, sample.ray, vInterval);

  if (!hasInterval(vInterval))
    return;

  VolumetricModel *varying model = vInterval.volumetricModel;

  Ray &ray = sample.ray;

  ray.t0 = vInterval.interval.lower;
  ray.t  = vInterval.interval.upper;

  const float jitter = LDSampler_getFloat(ldSampler, 0);

  foreach_unique(m in model)
  {     
    Volume *uniform volume       = m->volume;
    TransferFunction *uniform tf = m->transferFunction;

    uniform VKLIntervalIterator intervalIterator;
    vklInitIntervalIteratorV(&intervalIterator,
                             volume->vklVolume,
                             (varying vkl_vec3f *)&ray.org,
                             (varying vkl_vec3f *)&ray.dir,
                             (varying vkl_range1f *)&vInterval.interval,
                             m->vklValueSelector);

    VKLInterval interval;
    while (vklIterateIntervalV(&intervalIterator, &interval) && alpha < 0.99f) {
      const float nominalSamplingDt = interval.nominalDeltaT / m->samplingRate;

      // initial sub interval, based on our renderer-defined sampling rate
      // and the volume's nominal dt
      box1f subInterval = make_box1f(
          interval.tRange.lower,
          min(interval.tRange.lower + nominalSamplingDt, interval.tRange.upper));
 
      while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
        ray.t0  = subInterval.lower + jitter * (subInterval.upper - subInterval.lower);
        const float dt = subInterval.upper - subInterval.lower;

        // Get volume sample
        vec3f p = ray.org + ray.t0 * ray.dir;
        const float sample = vklComputeSampleV(volume->vklVolume, (const varying vkl_vec3f *uniform) &p);

        if (!isnan(sample)) {
          vec4f sampleColorOpacity = tf->get(tf, sample);
 
          const float clampedOpacity =
             clamp(sampleColorOpacity.w * dt);
          color = color
              + ((1.f - alpha) * clampedOpacity * make_vec3f(sampleColorOpacity));
          alpha = alpha + ((1.f - alpha) * clampedOpacity);
        }

        // compute next sub interval
        subInterval.lower = subInterval.upper;
        subInterval.upper =
            min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
      }
    }
  }
}

#define DEFINE_RAYCAST_RENDERER(name)                                    \
  export void *uniform RaycastRenderer_create_##name(void *uniform cppE) \
  {                                                                      \
    uniform RaycastRenderer *uniform self =                              \
        uniform new uniform RaycastRenderer;                             \
    Renderer_Constructor(&self->super, cppE);                            \
    self->super.renderSample = RaycastRenderer_renderSample_##name;      \
    return self;                                                         \
  }

DEFINE_RAYCAST_RENDERER(testFrame);
DEFINE_RAYCAST_RENDERER(rayDir);
DEFINE_RAYCAST_RENDERER(eyeLight);
DEFINE_RAYCAST_RENDERER(Ng);
DEFINE_RAYCAST_RENDERER(Ns);
DEFINE_RAYCAST_RENDERER(dPds);
DEFINE_RAYCAST_RENDERER(dPdt);
DEFINE_RAYCAST_RENDERER(eyeLight_primID);
DEFINE_RAYCAST_RENDERER(eyeLight_geomID);
DEFINE_RAYCAST_RENDERER(eyeLight_instID);
DEFINE_RAYCAST_RENDERER(eyeLight_vertexColor);
DEFINE_RAYCAST_RENDERER(backfacing_Ng);
DEFINE_RAYCAST_RENDERER(backfacing_Ns);
DEFINE_RAYCAST_RENDERER(volume);
