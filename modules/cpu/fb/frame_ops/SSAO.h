// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "camera/PerspectiveCamera.h"
#include "fb/FrameOp.h"
#include "rkcommon/tasking/parallel_for.h"
// std
#include <algorithm>

#ifdef OSPRAY_TARGET_SYCL
namespace ispc {
void *LiveSSAOFrameOp_create();
void LiveSSAOFrameOp_set(void *uniform _self,
    float nearClip,
    float ssaoStrength,
    void *_windowSize,
    void *_pixelSize,
    void *_cameraSpace,
    void *_kernel,
    void *_randomVecs);
void LiveSSAOFrameOp_getOcclusion(const void *_self,
    const void *_fb,
    float *occlusionBuffer,
    const float radius,
    const float checkRadius,
    unsigned int kernelSize,
    int programID);
void LiveSSAOFrameOp_applyOcclusion(
    void *_self, const void *_fb, void *_color, float *occlusionBuffer);
int8_t getProgramCount();
} // namespace ispc
#else
// ispc
#include "fb/frame_ops/SSAO_ispc.h"
#endif

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

  SSAOFrameOp(api::Device &device)
      : FrameOp(static_cast<api::ISPCDevice &>(device))
  {}
  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;
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
  void applySSAO(const FrameBufferView &fb, T *color, const Camera *);

  LiveSSAOFrameOp(api::ISPCDevice &device,
      FrameBufferView &fbView,
      void *,
      float,
      float,
      float,
      std::vector<vec3f>,
      std::vector<vec3f>);
  void process(void *, const Camera *) override;
};

// Inlined definitions ////////////////////////////////////////////////////////

template <typename T>
inline void LiveSSAOFrameOp::applySSAO(
    const FrameBufferView &fb, T *color, const Camera *cam)
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
