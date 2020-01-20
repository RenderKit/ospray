// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../FrameBufferView.ih"
#include "../Tile.ih"
// ospcommon
#include "math/LinearSpace.ih"
#include "math/vec.ih"

struct LiveSSAOFrameOp
{
  float nearClip;
  // The strength of ssao, range [0,1]
  float ssaoStrength;
  // The size of viewplane in worldspace
  vec2f windowSize;
  // The size of a screen pixel in worldspace
  vec2f pixelSize;

  // The transformation mat from world to camera
  AffineSpace3f cameraSpace;

  vec3f *kernel;
  vec3f *randomVecs;
};

export void *uniform LiveSSAOFrameOp_create()
{
  LiveSSAOFrameOp *uniform self = uniform new uniform LiveSSAOFrameOp;
  self->ssaoStrength = 1;
  self->windowSize.x = 0;
  self->windowSize.y = 0;
  self->pixelSize.x = 0;
  self->pixelSize.y = 0;

  return self;
}

export void LiveSSAOFrameOp_set(void *uniform _self,
    uniform float nearClip,
    uniform float ssaoStrength,
    void *uniform _windowSize,
    void *uniform _pixelSize,
    void *uniform _cameraSpace,
    void *uniform _kernel,
    void *uniform _randomVecs)
{
  LiveSSAOFrameOp *uniform self = (LiveSSAOFrameOp * uniform) _self;
  self->nearClip = nearClip;
  self->ssaoStrength = ssaoStrength;
  self->windowSize = *((vec2f * uniform) _windowSize);
  self->pixelSize = *((vec2f * uniform) _pixelSize);
  self->cameraSpace = *((AffineSpace3f * uniform) _cameraSpace);
  self->kernel = (vec3f * uniform) _kernel;
  self->randomVecs = (vec3f * uniform) _randomVecs;
}

export void LiveSSAOFrameOp_getOcclusion(const void *uniform _self,
    void *uniform _fb,
    float *uniform occlusionBuffer,
    const uniform float radius,
    const uniform float checkRadius,
    int uniform kernelSize,
    int uniform programID)
{
  const LiveSSAOFrameOp *uniform self = (const LiveSSAOFrameOp *uniform)_self;
  FrameBufferView uniform fb = *((FrameBufferView * uniform) _fb);
  uniform float *depthBuffer = fb.depthBuffer;

  int pixelID = programID * programCount + programIndex;

  int i = pixelID % fb.fbDims.x;
  int j = pixelID / fb.fbDims.x;
  float occlusion = 0.f;
  float pointDepth = depthBuffer[pixelID];
  float sampleDepth = 1.0f;
  if (occlusionBuffer[pixelID] >= 0) {
    vec3f pointNormal = fb.normalBuffer[pixelID];

    // project pixel from screenspace to cameraspace, then to worldspace
    vec3f pixelPosCamera =
        make_vec3f((i - fb.fbDims.x / 2.0) * self->pixelSize.x,
            (j - fb.fbDims.y / 2.0) * self->pixelSize.y,
            -self->nearClip);

    pixelPosCamera = pixelPosCamera * ((pointDepth) / (length(pixelPosCamera)));
    pixelPosCamera = xfmPoint(rcp(self->cameraSpace), pixelPosCamera);

    // apply kernal
    for (uint32 k = 0; k < kernelSize; ++k) {
      // calculate TBN matrix
      vec3f tangent = normalize(self->randomVecs[k]
          - pointNormal * dot(self->randomVecs[k], pointNormal));
      vec3f bitangent = cross(pointNormal, tangent);
      LinearSpace3f TBN = make_LinearSpace3f(tangent, bitangent, pointNormal);
      // get sample in worldspace
      vec3f samplePosCamera =
          pixelPosCamera + radius * xfmVector(TBN, self->kernel[k]);
      // project sample point back to cameraspace, then to screenspace
      samplePosCamera = xfmPoint((self->cameraSpace), samplePosCamera);

      vec2f samplePosScreen = make_vec2f(samplePosCamera.x / samplePosCamera.z
              * (-self->nearClip) / self->pixelSize.x,
          samplePosCamera.y / samplePosCamera.z * (-self->nearClip)
              / self->pixelSize.y);
      int samplePos_i = fb.fbDims.x / 2.0 + samplePosScreen.x;
      int samplePos_j = fb.fbDims.y / 2.0 + samplePosScreen.y;

      if (samplePos_i < 0 || samplePos_i >= fb.fbDims.x)
        continue;
      if (samplePos_j < 0 || samplePos_j >= fb.fbDims.y)
        continue;

      // get sample depth
      sampleDepth = depthBuffer[samplePos_j * fb.fbDims.x + samplePos_i];

      // range check & accumulate
      float rangeCheck =
          abs(pointDepth - sampleDepth) < checkRadius ? 1.0 : 0.0;

      occlusion = occlusion
          + (length(samplePosCamera) > sampleDepth ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1 - self->ssaoStrength * (occlusion / kernelSize);
    occlusionBuffer[pixelID] = occlusion;
  }
}

export void LiveSSAOFrameOp_applyOcclusion(void *uniform _self,
    void *uniform _fb,
    void *uniform _color,
    float *uniform occlusionBuffer)
{
  FrameBufferView *uniform fb = (FrameBufferView * uniform) _fb;
  for (uint32 pixelID = 0; pixelID < fb->fbDims.x * fb->fbDims.y; pixelID++) {
    if (occlusionBuffer[pixelID] >= 0) {
      for (uint32 c = 0; c < 3; ++c) {
        if ((fb->colorBufferFormat == ColorBufferFormat_RGB_UINT8)
            || (fb->colorBufferFormat == ColorBufferFormat_RGBA_UINT8)) {
          uint8 *uniform color = (uint8 * uniform) _color;
          color[pixelID * 4 + c] =
              color[pixelID * 4 + c] * (occlusionBuffer[pixelID]);
        } else if (fb->colorBufferFormat == ColorBufferFormat_RGBA_FLOAT32) {
          float *uniform color = (float *uniform)_color;
          color[pixelID * 4 + c] *= (occlusionBuffer[pixelID]);
        } else {
          // color format unknown
        }
      }
    }
  }
}

export int8 uniform getProgramCount()
{
  return programCount;
}
