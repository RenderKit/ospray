// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "RendererType.ih"

#ifdef __cplusplus
#include "common/StructShared.h"
#ifdef OSPRAY_TARGET_DPCPP
#include "camera/Camera.ih"
#include "camera/CameraShared.h"
#include "common/WorldShared.h"
#include "fb/FrameBufferShared.h"
#include "fb/TileShared.h"
#include "render/ScreenSample.ih"
#endif
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_DPCPP)
typedef void *Renderer_RenderSampleFct;
#else
#ifndef OSPRAY_TARGET_DPCPP
struct Renderer;
struct World;
struct Camera;
struct FrameBuffer;
struct ScreenSample;
struct Tile;
#endif

// Render a given screen sample (as specified in sampleID), and
// returns the radiance in 'retVal'. sampleID.x and .y refer to the
// pixel ID in the frame buffer, sampleID.z indicates that this should
// be the 'z'th sample in a sequence of samples for renderers that
// support multi-sampling and/or accumulation.
// Note that it is perfectly valid for different samples to have the
// same x, y, or even z values. For example, a accumulation-based
// renderer may issue chunks of pixels with sampleID.z all 0 in the
// first frame, sampleID.z all 1 in the second, etc, while a
// super-sampling renderer make issue a chunk of N samples together for
// the same pixel coordinates in the same call.
typedef void (*Renderer_RenderSampleFct)(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform model,
    void *uniform perFrameData,
    varying ScreenSample &retValue);
#endif

struct Texture2D;
struct Material;
struct PixelFilter;
struct MathConstants;

struct Renderer
{
  RendererType type;
  Renderer_RenderSampleFct renderSample;

  int32 spp;
  vec4f bgColor; // background color and alpha
  Texture2D *backplate;
  Texture2D *maxDepthTexture; // optional maximum depth texture used for early
                              // ray termination
  int maxDepth;
  float minContribution;

  int32 numMaterials;
  Material **material;

  PixelFilter *pixelFilter;
  MathConstants *mathConstants;

#ifdef __cplusplus
  Renderer()
      : type(RENDERER_TYPE_UNKNOWN),
        renderSample(nullptr),
        spp(1),
        bgColor(0.f),
        backplate(nullptr),
        maxDepthTexture(nullptr),
        maxDepth(20),
        minContribution(0.001f),
        numMaterials(0),
        material(nullptr),
        pixelFilter(nullptr),
        mathConstants(nullptr)
  {}
};
#ifdef OSPRAY_TARGET_DPCPP
SYCL_EXTERNAL void Renderer_default_renderSample(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform model,
    void *uniform perFrameData,
    varying ScreenSample &sample);

void Renderer_pick(const void *_self,
    const void *_fb,
    const void *_camera,
    const void *_world,
    const vec2f &screenPos,
    vec3f &pos,
    int32 &instID,
    int32 &geomID,
    int32 &primID,
    int32 &hit);
#endif
} // namespace ispc
#else
};
#endif // __cplusplus
