// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/FeatureFlagsEnum.h"
#include "common/Intersect.ih"
#include "common/World.ih"
#include "fb/FrameBuffer.ih"
#include "fb/FrameBufferDispatch.ih"
#include "fb/RenderTaskDesc.ih"
#include "math/random.ih"
#include "pf/PixelFilterDispatch.ih"
#include "render/Renderer.ih"
#include "render/ScreenSample.ih"
#include "rkcommon/utility/random.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "volume/VolumetricModel.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
#endif
// c++ shared
#include "DebugRendererShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

// common utility function, traces ray and handles default and background
inline bool hitBackground(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  traceRay(world, sample.ray, ffh);
  sample.z = sample.ray.t;
  sample.alpha = 1.f;

  sample.rgb = make_vec3f(Renderer_getBackground(self, sample.pos, ffh));

  return noHit(sample.ray);
}

inline float eyeLight(varying ScreenSample &sample)
{
  return 0.2f + 0.8f * abs(dot(normalize(sample.ray.Ng), sample.ray.dir));
}

/* a simple test-frame renderer that doesn't even trace a ray, just
  returns a well-defined test frame (mostly useful for debugging
  whether frame buffers are properly set up etcpp */
static void DebugRenderer_testFrame(
    Renderer *uniform, World *uniform, varying ScreenSample &sample)
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
static void DebugRenderer_rayDir(
    Renderer *uniform, World *uniform, varying ScreenSample &sample)
{
  sample.rgb = absf(sample.ray.dir);
  sample.alpha = 1.f;
  sample.z = 1.f;
}

static void DebugRenderer_eyeLight(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh)) {
    return;
  }
  sample.rgb = make_vec3f(eyeLight(sample));
}

static void DebugRenderer_Ng(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NG, ffh);
  sample.rgb = absf(dg.Ng);
}

static void DebugRenderer_Ns(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NS, ffh);
  sample.rgb = absf(dg.Ns);
}

static void DebugRenderer_texCoord(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;
  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TEXCOORD, ffh);
  sample.rgb = abs(make_vec3f(dg.st.x, dg.st.y, 0.0f));
}

static void DebugRenderer_dPds(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;
  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TANGENTS, ffh);
  sample.rgb = normalize(dg.dPds);
  if (sample.rgb.x < 0.f)
    sample.rgb.x = sample.rgb.x * -0.3f;
  if (sample.rgb.y < 0.f)
    sample.rgb.y = sample.rgb.y * -0.3f;
  if (sample.rgb.z < 0.f)
    sample.rgb.z = sample.rgb.z * -0.3f;
}

static void DebugRenderer_dPdt(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_TANGENTS, ffh);
  sample.rgb = normalize(dg.dPdt);
  if (sample.rgb.x < 0.f)
    sample.rgb.x = sample.rgb.x * -0.3f;
  if (sample.rgb.y < 0.f)
    sample.rgb.y = sample.rgb.y * -0.3f;
  if (sample.rgb.z < 0.f)
    sample.rgb.z = sample.rgb.z * -0.3f;
}

static void DebugRenderer_vertexColor(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_COLOR | DG_NS, ffh);
  sample.rgb = make_vec3f(dg.color)
      * abs(dot(normalize(sample.ray.dir), normalize(dg.Ns)));
}

static void DebugRenderer_primID(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  sample.rgb = eyeLight(sample) * makeRandomColor(sample.ray.primID);
}

static void DebugRenderer_instID(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  sample.rgb = eyeLight(sample) * makeRandomColor(sample.ray.instID);
}

static void DebugRenderer_geomID(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  sample.rgb = eyeLight(sample) * makeRandomColor(sample.ray.geomID);
}

static void DebugRenderer_backfacing_Ng(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  sample.rgb = make_vec3f(eyeLight(sample));
  if (dot(sample.ray.Ng, sample.ray.dir) > 0.f)
    sample.rgb.y = 0.f;
}

static void DebugRenderer_backfacing_Ns(Renderer *uniform self,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  if (hitBackground(self, world, sample, ffh))
    return;

  DifferentialGeometry dg;
  postIntersect(world, self, dg, sample.ray, DG_NORMALIZE | DG_NS, ffh);
  const float c = dot(dg.Ns, sample.ray.dir);
  sample.rgb = make_vec3f(.2f + .8f * abs(c));
  if (c > 0.f)
    sample.rgb.y = 0.f;
}

static void DebugRenderer_volume(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
#ifdef OSPRAY_ENABLE_VOLUMES
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  if (!(ff.other & FFO_VOLUME_IN_SCENE)) {
    return;
  }

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      self->mathConstants,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  vec3f &color = sample.rgb;
  float &alpha = sample.alpha;

  vec4f bgColor = Renderer_getBackground(self, sample.pos, ffh);
  color = make_vec3f(bgColor);
  alpha = bgColor.w;

  VolumeInterval vInterval;
  traceVolumeRay(world, sample.ray, vInterval);

  if (!hasInterval(vInterval)) {
    return;
  }

  VolumetricModel *varying model = vInterval.volumetricModel;

  Ray &ray = sample.ray;

  ray.t0 = vInterval.interval.lower;
  ray.t = vInterval.interval.upper;

  const float jitter = LDSampler_getFloat(ldSampler, 0);

  uniform uint8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
#ifdef OSPRAY_TARGET_SYCL
  {
    VolumetricModel *uniform m = model;
#else
  foreach_unique (m in model) {
#endif
    Volume *uniform volume = m->volume;

    float time = 0.5f;
#if OPENVKL_VERSION_MAJOR == 1
    // We know if we have OpenVKL version 1 we're not targetting SYCL
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIteratorV(m->vklIntervalContext,
            (varying vkl_vec3f *)&ray.org,
            (varying vkl_vec3f *)&ray.dir,
            (varying vkl_range1f *)&vInterval.interval,
            &time,
            (void *uniform)intervalIteratorBuffer);
#else
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIteratorV(&m->vklIntervalContext,
            (varying vkl_vec3f *)&ray.org,
            (varying vkl_vec3f *)&ray.dir,
            (varying vkl_range1f *)&vInterval.interval,
#ifdef OSPRAY_TARGET_SYCL
            time,
#else
            &time,
#endif
            (void *uniform)intervalIteratorBuffer
#ifdef OSPRAY_TARGET_SYCL
            ,
            ff.volume
#endif
        );
#endif
    VKLInterval interval;

    static const uniform float samplingRate = 0.5f;

    while (vklIterateIntervalV(intervalIterator,
               &interval
#ifdef OSPRAY_TARGET_SYCL
               ,
               ff.volume
#endif
               )
        && alpha < 0.99f) {
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
#if OPENVKL_VERSION_MAJOR == 1
        // We know if we have OpenVKL version 1 we're not targetting SYCL
        const float sample = vklComputeSampleV(
            volume->vklSampler, (const varying vkl_vec3f *uniform) & p);
#else
        const float sample = vklComputeSampleV(&volume->vklSampler,
            (const varying vkl_vec3f *uniform) & p
#ifdef OSPRAY_TARGET_SYCL
            ,
            0,
            time,
            ff.volume
#endif
        );
#endif
        if (!isnan(sample)) {
          vec4f sampleColorOpacity =
              TransferFunction_dispatch_get(m->transferFunction, sample);

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
#endif
}

static void DebugRenderer_renderSample(Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
#ifdef OSPRAY_TARGET_SYCL
  const DebugRendererType debugType =
      ffh.kernel_handler.get_specialization_constant<debugRendererType>();
#else
  DebugRenderer *uniform self = (DebugRenderer * uniform) _self;
  const DebugRendererType debugType = self->type;
#endif
  switch (debugType) {
  case TEST_FRAME:
    DebugRenderer_testFrame(_self, world, sample);
    break;
  case RAY_DIR:
    DebugRenderer_rayDir(_self, world, sample);
    break;
  case EYE_LIGHT:
    DebugRenderer_eyeLight(_self, world, sample, ffh);
    break;
  case NG:
    DebugRenderer_Ng(_self, world, sample, ffh);
    break;
  case NS:
    DebugRenderer_Ns(_self, world, sample, ffh);
    break;
  case COLOR:
    DebugRenderer_vertexColor(_self, world, sample, ffh);
    break;
  case TEX_COORD:
    DebugRenderer_texCoord(_self, world, sample, ffh);
    break;
  case DPDS:
    DebugRenderer_dPds(_self, world, sample, ffh);
    break;
  case DPDT:
    DebugRenderer_dPdt(_self, world, sample, ffh);
    break;
  case PRIM_ID:
    DebugRenderer_primID(_self, world, sample, ffh);
    break;
  case GEOM_ID:
    DebugRenderer_geomID(_self, world, sample, ffh);
    break;
  case INST_ID:
    DebugRenderer_instID(_self, world, sample, ffh);
    break;
  case BACKFACING_NG:
    DebugRenderer_backfacing_Ng(_self, world, sample, ffh);
    break;
  case BACKFACING_NS:
    DebugRenderer_backfacing_Ns(_self, world, sample, ffh);
    break;
  case VOLUME:
    DebugRenderer_volume(_self, fb, world, sample, ffh);
    break;
  default:
    DebugRenderer_testFrame(_self, world, sample);
    break;
  };
}

#define renderSampleFn DebugRenderer_renderSample
#include "render/RendererRenderTaskFn.inl"
#undef renderSampleFn

#ifdef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL void DebugRenderer_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh)
{
  Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, taskIndex0, ffh);
}
#else
export void DebugRenderer_renderTasks(void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    void *uniform _taskIDs,
    uniform uint32 numTasks)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  FrameBuffer *uniform fb = (FrameBuffer * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  World *uniform world = (World * uniform) _world;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  uniform FeatureFlagsHandler ffh;
  launch[numTasks] Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, ffh);
  sync;
}
#endif

OSPRAY_END_ISPC_NAMESPACE
