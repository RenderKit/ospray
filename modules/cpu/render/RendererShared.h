// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
typedef void *Renderer_RenderSampleFct;
#else
struct Renderer;
struct World;
struct Camera;
struct FrameBuffer;
struct ScreenSample;
struct Tile;

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
#endif // __cplusplus

struct Texture2D;
struct Material;
struct PixelFilter;

struct Renderer
{
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

#ifdef __cplusplus
  Renderer()
      : renderSample(nullptr),
        spp(1),
        bgColor(0.f),
        backplate(nullptr),
        maxDepthTexture(nullptr),
        maxDepth(20),
        minContribution(.001f),
        numMaterials(0),
        material(nullptr),
        pixelFilter(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
