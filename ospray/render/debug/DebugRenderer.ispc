// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "camera/PerspectiveCamera.ih"
#include "common/World.ih"
#include "fb/FrameBuffer.ih"
#include "math/random.ih"
#include "render/Renderer.ih"
#include "render/util.ih"
#include "volume/VolumetricModel.ih"

#include "DebugRendererType.h"

#include "openvkl/openvkl.isph"

// common utility function, traces ray and handles default and background
bool hitBackground(
    Renderer *uniform self, World *uniform world, varying ScreenSample &sample)
{
  traceRay(world, sample.ray);
  sample.z = sample.ray.t;
  sample.alpha = 1.f;

  sample.rgb = make_vec3f(Renderer_getBackground(self, sample.pos));

  return noHit(sample.ray);
}

float eyeLight(varying ScreenSample &sample)
{
  return 0.2f + 0.8f * dot(normalize(sample.ray.Ng), sample.ray.dir);
}

/* a simple test-frame renderer that doesn't even trace a ray, just
  returns a well-defined test frame (mostly useful for debugging
  whether frame buffers are properly set up etcpp */
void DebugRenderer_testFrame(Renderer *uniform,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  sample.rgb.x = ((sample.sampleID.x) % 256) / 255.f;
  sample.rgb.y = ((sample.sampleID.y) % 256) / 255.f;
  sample.rgb.z =
      ((sample.sampleID.x + sample.sampleID.y + sample.sampleID.z) % 256)
      / 255.f;
  sample.alpha = 1.f;
  sample.z = 1.f;
}

/* a simple test-frame renderer that doesn't even trace a ray, just
  returns the absolute of the ray direction */
void DebugRenderer_rayDir(Renderer *uniform,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  sample.rgb = absf(sample.ray.dir);
  sample.alpha = 1.f;
  sample.z = 1.f;
}

void DebugRenderer_eyeLight(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  sample.rgb = make_vec3f(eyeLight(sample));
}

void DebugRenderer_Ng(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NG);
  sample.rgb = absf(dg.Ng);
}

void DebugRenderer_Ns(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NS);
  sample.rgb = absf(dg.Ns);
}

void DebugRenderer_texCoord(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;
  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TEXCOORD);
  sample.rgb = abs(make_vec3f(dg.st.x, dg.st.y, 0.0f));
}

void DebugRenderer_dPds(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;
  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TANGENTS);
  sample.rgb = normalize(dg.dPds);
  if (sample.rgb.x < 0.f)
    sample.rgb.x = sample.rgb.x * -0.3f;
  if (sample.rgb.y < 0.f)
    sample.rgb.y = sample.rgb.y * -0.3f;
  if (sample.rgb.z < 0.f)
    sample.rgb.z = sample.rgb.z * -0.3f;
}

void DebugRenderer_dPdt(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TANGENTS);
  sample.rgb = normalize(dg.dPdt);
  if (sample.rgb.x < 0.f)
    sample.rgb.x = sample.rgb.x * -0.3f;
  if (sample.rgb.y < 0.f)
    sample.rgb.y = sample.rgb.y * -0.3f;
  if (sample.rgb.z < 0.f)
    sample.rgb.z = sample.rgb.z * -0.3f;
}

void DebugRenderer_vertexColor(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_COLOR | DG_NS);
  sample.rgb = make_vec3f(dg.color)
      * abs(dot(normalize(sample.ray.dir), normalize(dg.Ns)));
}

void DebugRenderer_primID(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  sample.rgb = eyeLight(sample) * make_random_color(sample.ray.primID);
}

void DebugRenderer_instID(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  sample.rgb = eyeLight(sample) * make_random_color(sample.ray.instID);
}

void DebugRenderer_geomID(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  sample.rgb = eyeLight(sample) * make_random_color(sample.ray.geomID);
}

void DebugRenderer_backfacing_Ng(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  sample.rgb = make_vec3f(eyeLight(sample));
  if (dot(sample.ray.Ng, sample.ray.dir) > 0.f)
    sample.rgb.y = 0.f;
}

void DebugRenderer_backfacing_Ns(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NS);
  const float c = dot(dg.Ns, sample.ray.dir);
  sample.rgb = make_vec3f(.2f + .8f * abs(c));
  if (c > 0.f)
    sample.rgb.y = 0.f;
}

void DebugRenderer_volume(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  if (hitBackground(self, world, sample))
    return;

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  vec3f &color = sample.rgb;
  float &alpha = sample.alpha;

  vec4f bgColor = Renderer_getBackground(self, sample.pos);
  color = make_vec3f(bgColor);
  alpha = bgColor.w;

  VolumeInterval vInterval;
  traceVolumeRay(world, sample.ray, vInterval);

  if (!hasInterval(vInterval))
    return;

  VolumetricModel *varying model = vInterval.volumetricModel;

  Ray &ray = sample.ray;

  ray.t0 = vInterval.interval.lower;
  ray.t = vInterval.interval.upper;

  const float jitter = LDSampler_getFloat(ldSampler, 0);

  foreach_unique (m in model) {
    Volume *uniform volume = m->volume;
    TransferFunction *uniform tf = m->transferFunction;

    VKLIntervalIterator intervalIterator;
    vklInitIntervalIteratorV(&intervalIterator,
        volume->vklVolume,
        (varying vkl_vec3f *)&ray.org,
        (varying vkl_vec3f *)&ray.dir,
        (varying vkl_range1f *)&vInterval.interval,
        m->vklValueSelector);

    VKLInterval interval;

    static const uniform float samplingRate = 0.5f;

    while (vklIterateIntervalV(&intervalIterator, &interval) && alpha < 0.99f) {
      const float nominalSamplingDt = interval.nominalDeltaT / samplingRate;

      // initial sub interval, based on our renderer-defined sampling rate
      // and the volume's nominal dt
      box1f subInterval = make_box1f(interval.tRange.lower,
          min(interval.tRange.lower + nominalSamplingDt,
              interval.tRange.upper));

      while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
        ray.t0 = subInterval.lower
            + jitter * (subInterval.upper - subInterval.lower);
        const float dt = subInterval.upper - subInterval.lower;

        // Get volume sample
        vec3f p = ray.org + ray.t0 * ray.dir;
        const float sample = vklComputeSampleV(
            volume->vklVolume, (const varying vkl_vec3f *uniform) & p);

        if (!isnan(sample)) {
          vec4f sampleColorOpacity = tf->get(tf, sample);

          const float clampedOpacity = clamp(sampleColorOpacity.w * dt);

          color = color
              + ((1.f - alpha) * clampedOpacity
                  * make_vec3f(sampleColorOpacity));
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

// DebugRenderer C++ interface ////////////////////////////////////////////////

export void *uniform DebugRenderer_create(void *uniform cppE)
{
  Renderer *uniform self = uniform new Renderer;
  Renderer_Constructor(self, cppE);
  self->renderSample = DebugRenderer_testFrame;
  return self;
}

export void DebugRenderer_set(void *uniform _self, uniform int type)
{
  Renderer *uniform self = (Renderer * uniform) _self;

  switch (type) {
  case RAY_DIR:
    self->renderSample = DebugRenderer_rayDir;
    break;
  case EYE_LIGHT:
    self->renderSample = DebugRenderer_eyeLight;
    break;
  case NG:
    self->renderSample = DebugRenderer_Ng;
    break;
  case NS:
    self->renderSample = DebugRenderer_Ns;
    break;
  case COLOR:
    self->renderSample = DebugRenderer_vertexColor;
    break;
  case TEX_COORD:
    self->renderSample = DebugRenderer_texCoord;
    break;
  case DPDS:
    self->renderSample = DebugRenderer_dPds;
    break;
  case DPDT:
    self->renderSample = DebugRenderer_dPdt;
    break;
  case PRIM_ID:
    self->renderSample = DebugRenderer_primID;
    break;
  case GEOM_ID:
    self->renderSample = DebugRenderer_geomID;
    break;
  case INST_ID:
    self->renderSample = DebugRenderer_instID;
    break;
  case BACKFACING_NG:
    self->renderSample = DebugRenderer_backfacing_Ng;
    break;
  case BACKFACING_NS:
    self->renderSample = DebugRenderer_backfacing_Ns;
    break;
  case VOLUME:
    self->renderSample = DebugRenderer_volume;
    break;
  case TEST_FRAME:
  default:
    self->renderSample = DebugRenderer_testFrame;
    break;
  }
}
