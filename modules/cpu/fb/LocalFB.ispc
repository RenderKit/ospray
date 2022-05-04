// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameBuffer.ih"
#include "fb/Tile.ih"
// c++ shared
#include "LocalFBShared.h"

// write tile into the given frame buffer's color buffer,
// this buffer _must_ exist when this fct is called, and it
// _must_ have format 'name'
#define template_writeTile(name, type, cvt)                                    \
  export void LocalFrameBuffer_writeTile_##name(                               \
      void *uniform _fb, const void *uniform _tile)                            \
  {                                                                            \
    Tile *uniform tile = (Tile * uniform) _tile;                               \
    uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;             \
    uniform type *uniform color = (uniform type * uniform) fb->colorBuffer;    \
    if (!color)                                                                \
      /* actually, this should never happen ... */                             \
      return;                                                                  \
                                                                               \
    color += (uniform uint64)tile->region.lower.y * fb->super.size.x;          \
    VaryingTile *uniform varyTile = (VaryingTile * uniform) tile;              \
    for (uniform uint32 iiy = tile->region.lower.y;                            \
         iiy < tile->region.upper.y;                                           \
         iiy++) {                                                              \
      uniform uint32 chunkID =                                                 \
          (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);           \
      for (uint32 iix = tile->region.lower.x + programIndex;                   \
           iix < tile->region.upper.x;                                         \
           iix += programCount, chunkID++) {                                   \
        unmasked                                                               \
        {                                                                      \
          varying vec4f col = make_vec4f(varyTile->r[chunkID],                 \
              varyTile->g[chunkID],                                            \
              varyTile->b[chunkID],                                            \
              varyTile->a[chunkID]);                                           \
                                                                               \
          const type cvtCol = cvt(col);                                        \
        }                                                                      \
        color[iix] = cvtCol;                                                   \
      }                                                                        \
      color += fb->super.size.x;                                               \
    }                                                                          \
  }

template_writeTile(RGBA8, uint32, cvt_uint32);
template_writeTile(SRGBA, uint32, linear_to_srgba8);
inline vec4f cvt_nop(const vec4f &v)
{
  return v;
};
template_writeTile(RGBA32F, vec4f, cvt_nop);
#undef template_writeTile

// Write the samples into the given frame buffers color buffer in the color
// buffer's format
#define template_writeSample(name, type, cvt)                                  \
  inline void LocalFB_writeSample_##name(                                      \
      LocalFB *uniform fb, vec4f &col, const int pixelID)                      \
  {                                                                            \
    uniform type *uniform color = (uniform type * uniform) fb->colorBuffer;    \
    if (!color)                                                                \
      /* actually, this should never happen ... */                             \
      return;                                                                  \
                                                                               \
    const type cvtCol = cvt(col);                                              \
    color[pixelID] = cvtCol;                                                   \
  }

template_writeSample(RGBA8, uint32, cvt_uint32);
template_writeSample(SRGBA, uint32, linear_to_srgba8);
template_writeSample(RGBA32F, vec4f, cvt_nop);
#undef template_writeSample

/* Accumulates the screenSample color and alpha into the framebuffer's
 * accumulation buffer. The accumulation buffer will store
 *
 *     prevAccum + sampleValue
 *
 * while the returned current accumulated color will be
 *
 *    (prevAccum + sampleValue) / numAccumulated
 *
 * I.e., the color to be written to the final framebuffer for display for
 * the current frame
 *
 * Variance and error computation based based on
 * Dammertz et al., "A Hierarchical Automatic Stopping Condition for Monte Carlo
 * Global Illumination", WSCG 2010
 */
inline vec4f LocalFB_accumulateColorSample(LocalFB *uniform fb,
    const ScreenSample &screenSample,
    const int pixelID,
    uniform RenderTaskDesc &taskDesc)
{
  uniform vec4f *uniform accum = fb->accumBuffer;
  uniform vec4f *uniform variance = fb->varianceBuffer;
  const uniform float accScale = rcpf(taskDesc.accumID + 1);
  const uniform float accHalfScale = rcpf(taskDesc.accumID / 2 + 1);

  const vec4f sampleValue = make_vec4f(screenSample.rgb, screenSample.alpha);

  vec4f acc = make_vec4f(0.f);
  if (taskDesc.accumID > 0) {
    acc = accum[pixelID];
  }
  acc = acc + sampleValue;
  accum[pixelID] = acc;

  acc = acc * accScale;

  // Variance and error buffers accumulate every other frame to ensure we get a
  // monotonically decreasing sequence of error values
  if (variance && (taskDesc.accumID & 1)) {
    vec4f vari = make_vec4f(0.f);
    if (taskDesc.accumID > 1) {
      vari = variance[pixelID];
    }
    vari = vari + sampleValue;
    variance[pixelID] = vari;

    float err = 0.f;
    // Invert alpha (bright alpha is more important)
    const float den2 = reduce_add(make_vec3f(acc)) + (1.f - acc.w);
    if (den2 > 0.f) {
      const vec4f diff = absf(acc - accHalfScale * vari);
      err += reduce_add(diff) * rsqrtf(den2);
    }
    // We sum over the task because we may have more pixels in the task than
    // the SIMD width
    taskDesc.error += reduce_add(err);
  }
  return acc;
}

inline void LocalFB_accumulateDepth(LocalFB *uniform fb,
    const float &sampleDepth,
    const int pixelID,
    const uniform int accumID)
{
  uniform float *uniform depth = (uniform float *uniform)fb->depthBuffer;

  float acc = accumID > 0 ? depth[pixelID] : inf;
  depth[pixelID] = min(acc, sampleDepth);
}

inline void LocalFB_accumulateAux(uniform vec3f *uniform auxBuffer,
    const vec3f &sampleAux,
    const int pixelID,
    const uniform int accumID)
{
  const uniform float accScale = rcpf(accumID + 1);

  vec3f acc = accumID > 0 ? auxBuffer[pixelID] : make_vec3f(0.f);
  acc = (acc * accumID + sampleAux) * accScale;
  auxBuffer[pixelID] = acc;
}

/* Write the samples into the framebuffer. Accumulates color (if accumulation is
 * enabled) and writes aux channels (depth, normal, albedo) that the framebuffer
 * has and writes the accumulated color in to the framebuffer for display.
 */
void LocalFB_accumulateSample(FrameBuffer *uniform _fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &taskDesc)
{
  LocalFB *uniform fb = (LocalFB * uniform) _fb;
  const int pixelID =
      screenSample.sampleID.x + fb->super.size.x * screenSample.sampleID.y;

  // Default final color is the current sample color, used if we don't have
  // accumulation enabled
  vec4f finalColor = make_vec4f(screenSample.rgb, screenSample.alpha);
  if (fb->accumBuffer) {
    finalColor =
        LocalFB_accumulateColorSample(fb, screenSample, pixelID, taskDesc);
  }

  float depth = inf;
  if (fb->depthBuffer) {
    LocalFB_accumulateDepth(fb, screenSample.z, pixelID, taskDesc.accumID);

    depth = fb->depthBuffer[pixelID];
  }

  vec3f normal = make_vec3f(0.f);
  if (fb->normalBuffer) {
    LocalFB_accumulateAux(
        fb->normalBuffer, screenSample.normal, pixelID, taskDesc.accumID);

    normal = fb->normalBuffer[pixelID];
  }

  vec3f albedo = make_vec3f(0.f);
  if (fb->albedoBuffer) {
    LocalFB_accumulateAux(
        fb->albedoBuffer, screenSample.albedo, pixelID, taskDesc.accumID);

    albedo = fb->albedoBuffer[pixelID];
  }

  if (fb->primitiveIDBuffer) {
    if (taskDesc.accumID == 0)
      fb->primitiveIDBuffer[pixelID] = screenSample.primID;
  }

  if (fb->objectIDBuffer) {
    if (taskDesc.accumID == 0)
      fb->objectIDBuffer[pixelID] = screenSample.geomID;
  }

  if (fb->instanceIDBuffer) {
    if (taskDesc.accumID == 0)
      fb->instanceIDBuffer[pixelID] = screenSample.instID;
  }

  // Run any pixel ops for this pixel
  // TODO: Best as inline in this kernel? Or a separate kernel?
  finalColor =
      FrameBuffer_runPixelOps(&fb->super, finalColor, depth, normal, albedo);

  // Write the final color out to the color buffer in the color format
  // requested by the app
  if (fb->colorBuffer) {
    switch (fb->super.colorBufferFormat) {
    case OSP_FB_RGBA8:
      LocalFB_writeSample_RGBA8(fb, finalColor, pixelID);
      break;
    case OSP_FB_SRGBA:
      LocalFB_writeSample_SRGBA(fb, finalColor, pixelID);
      break;
    case OSP_FB_RGBA32F:
      LocalFB_writeSample_RGBA32F(fb, finalColor, pixelID);
      break;
    default:
      // Nothing to do for color format none
      break;
    }
  }

  FrameBuffer_updateProgress(&fb->super, popcnt(lanemask()));
}

uniform RenderTaskDesc LocalFB_getRenderTaskDesc(
    FrameBuffer *uniform _fb, const uniform uint32 taskID)
{
  LocalFB *uniform fb = (LocalFB * uniform) _fb;
  uniform RenderTaskDesc desc;
  desc.taskID = taskID;
  desc.accumID = fb->taskAccumID ? fb->taskAccumID[taskID] : 0;
  desc.error = 0.f;

  uniform vec2i renderTaskSize = fb->super.renderTaskSize;
  uniform vec2i taskStart =
      make_vec2i(taskID % fb->numRenderTasks.x, taskID / fb->numRenderTasks.x);
  desc.region.lower = taskStart * renderTaskSize;
  desc.region.upper = min(desc.region.lower + renderTaskSize, fb->super.size);
  return desc;
}

void LocalFB_completeTask(
    FrameBuffer *uniform _fb, const uniform RenderTaskDesc &taskDesc)
{
  LocalFB *uniform fb = (LocalFB * uniform) _fb;

  // Write error for the task every other frame to get a monotonically
  // decreasing sequence of error values
  if (fb->varianceBuffer && (taskDesc.accumID & 1)) {
    // Scale to be backwards compatible with the old default tile/task
    // size of 64x64
    const uniform vec2i dia = box_size(taskDesc.region);
    const uniform float taskSize = (uniform float)dia.x * dia.y;
    fb->taskRegionError[taskDesc.taskID] =
        taskDesc.error * rcp(taskSize) * 64.f;
  }

  // Increment accumID for this task
  if (fb->taskAccumID) {
    fb->taskAccumID[taskDesc.taskID] = taskDesc.accumID + 1;
  }
}

// Write the input tile into the row-major framebuffer's depth buffer
export void LocalFrameBuffer_writeDepthTile(
    void *uniform _fb, const void *uniform _tile)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;

  VaryingTile *uniform varyTile = (VaryingTile * uniform) tile;
  uniform float *uniform depth = (uniform float *uniform)fb->depthBuffer;
  depth += (uniform uint64)tile->region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile->region.lower.y; iiy < tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < tile->region.upper.x;
         iix += programCount, chunkID++) {
      depth[iix] = varyTile->z[chunkID];
    }
    depth += fb->super.size.x;
  }
}

// "accumulate" first frame only and write into buffer
export void LocalFrameBuffer_writeIDTile(void *uniform _fb,
    const void *uniform _tile,
    uniform uint32 *uniform dst,
    const void *uniform src)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;
  VaryingTile *uniform varyTile = (VaryingTile * uniform) tile;
  uniform uint32 *uniform buffer =
      dst; //(uniform uint32 *uniform)fb->primitiveIDBuffer;
  buffer += (uniform uint64)tile->region.lower.y * fb->super.size.x;
  const void *uniform upid = src; //(const void* uniform)tile->pid;
  const varying int32 *uniform pid = (const varying int32 *uniform)upid;

  for (uniform uint32 iiy = tile->region.lower.y; iiy < tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < tile->region.upper.x;
         iix += programCount, chunkID++) {
      if (tile->accumID == 0) {
        varying uint32 val = -1;
        unmasked
        {
          val = pid[chunkID];
        }
        buffer[iix] = val;
      }
    }
    buffer += fb->super.size.x;
  }
}

// Write a Tile's auxiliary buffer into the framebuffer's row major storage
export void LocalFrameBuffer_writeAuxTile(void *uniform _fb,
    const void *uniform _tile,
    uniform vec3f *uniform aux,
    const void *uniform _ax,
    const void *uniform _ay,
    const void *uniform _az)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;
  const varying float *uniform ax = (const varying float *uniform)_ax;
  const varying float *uniform ay = (const varying float *uniform)_ay;
  const varying float *uniform az = (const varying float *uniform)_az;

  const uniform float accumID = tile->accumID;
  const uniform float accScale = rcpf(tile->accumID + 1);
  aux += (uniform uint64)tile->region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile->region.lower.y; iiy < tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < tile->region.upper.x;
         iix += programCount, chunkID++) {
      aux[iix] = make_vec3f(ax[chunkID], ay[chunkID], az[chunkID]);
    }
    aux += fb->super.size.x;
  }
}

export void *uniform LocalFrameBuffer_accumulateSample_addr()
{
  return LocalFB_accumulateSample;
}

export void *uniform LocalFrameBuffer_getRenderTaskDesc_addr()
{
  return LocalFB_getRenderTaskDesc;
}

export void *uniform LocalFrameBuffer_completeTask_addr()
{
  return LocalFB_completeTask;
}
