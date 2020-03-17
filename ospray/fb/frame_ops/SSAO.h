// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../ImageOp.h"
#include "camera/PerspectiveCamera.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>
// ispc
#include "SSAO_ispc.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE SSAOFrameOp : public FrameOp
{
  float nearClip;
  float ssaoStrength;
  float radius, checkRadius;
  vec2f windowSize, pixelSize;
  AffineSpace3f cameraSpace;

  std::vector<vec3f> kernel;
  std::vector<vec3f> randomVecs;

  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;
  void commit() override;
  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveSSAOFrameOp : public LiveFrameOp
{
  void *ispcEquiv;
  float ssaoStrength;
  float radius, checkRadius;
  std::vector<vec3f> kernel;
  std::vector<vec3f> randomVecs;

  template <typename T>
  void applySSAO(FrameBufferView &fb, T *color, const Camera *);

  LiveSSAOFrameOp(FrameBufferView &fbView,
      void *,
      float,
      float,
      float,
      std::vector<vec3f>,
      std::vector<vec3f>);
  void process(const Camera *) override;
};

// Inlined definitions ////////////////////////////////////////////////////////

template <typename T>
inline void LiveSSAOFrameOp::applySSAO(
    FrameBufferView &fb, T *color, const Camera *cam)
{
  if (cam->toString().compare("ospray::PerspectiveCamera"))
    throw std::runtime_error(
        "SSAO fame operation must be used with perspective camera ("
        + cam->toString() + ")");

  PerspectiveCamera *camera = (PerspectiveCamera *)(cam);

  // get camera
  vec3f eyePos = camera->pos;
  vec3f lookDir = camera->dir;
  vec3f upDir = camera->up;
  float nearClip = camera->nearClip;

  // get cameraSpace matrix
  vec3f zaxis = normalize(-lookDir);
  vec3f xaxis = normalize(cross(upDir, zaxis));
  vec3f yaxis = cross(zaxis, xaxis);
  vec3f p_component =
      vec3f(-dot(eyePos, xaxis), -dot(eyePos, yaxis), -dot(eyePos, zaxis));
  LinearSpace3f cameraModelRot(xaxis, yaxis, zaxis);
  AffineSpace3f cameraSpace =
      (AffineSpace3f(cameraModelRot.transposed(), p_component));

  vec2f windowSize = fb.fbDims;
  vec2f pixelSize;
  // get screen pixel size in camera space
  float fovy = camera->fovy;
  pixelSize.y = nearClip * 2.f * tanf(deg2rad(0.5f * fovy)) / windowSize.y;
  pixelSize.x = pixelSize.y;

  ispc::LiveSSAOFrameOp_set(ispcEquiv,
      nearClip,
      ssaoStrength,
      &windowSize,
      &pixelSize,
      &cameraSpace,
      &kernel[0],
      &randomVecs[0]);

  float *depthBuffer = fb.depthBuffer;
  std::vector<float> occlusionBuffer(fb.fbDims.x * fb.fbDims.y, 0);
  tasking::parallel_for(fb.fbDims.x * fb.fbDims.y, [&](int pixelID) {
    if (std::isinf(depthBuffer[pixelID]))
      occlusionBuffer[pixelID] = -1;
  });

  int programcount = ispc::getProgramCount();
  tasking::parallel_for(
      fb.fbDims.x * fb.fbDims.y / programcount, [&](int programID) {
        ispc::LiveSSAOFrameOp_getOcclusion(ispcEquiv,
            &fb,
            &occlusionBuffer[0],
            radius,
            checkRadius,
            kernel.size(),
            programID);
      });

  ispc::LiveSSAOFrameOp_applyOcclusion(
      ispcEquiv, &fb, color, &occlusionBuffer[0]);
}

} // namespace ospray
