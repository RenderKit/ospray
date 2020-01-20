// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../FrameBuffer.h"
#include "SSAO_ispc.h"
#include "camera/PerspectiveCamera.h"
#include "common/Util.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

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

// Definitions //////////////////////////////////////////////////////////////

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

std::unique_ptr<LiveImageOp> SSAOFrameOp::attach(FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "ssao frame operation must be attached to framebuffer with color "
        "data");
  }

  if (!fbView.normalBuffer) {
    throw std::runtime_error(
        "SSAO frame operation must be attached to framebuffer with normal "
        "data");
  }

  if (!fbView.depthBuffer) {
    throw std::runtime_error(
        "SSAO frame operation must be attached to framebuffer with depth "
        "data");
  }

  void *ispcEquiv = ispc::LiveSSAOFrameOp_create();

  return ospcommon::make_unique<LiveSSAOFrameOp>(
      fbView, ispcEquiv, ssaoStrength, radius, checkRadius, kernel, randomVecs);
}

std::string SSAOFrameOp::toString() const
{
  return "ospray::SSAOFrameOp";
}

void SSAOFrameOp::commit()
{
  ssaoStrength = getParam<float>("strength", 1.f);
  windowSize = getParam<vec2f>("windowSize", vec2f(-1.f));
  radius = getParam<float>("radius", 0.3f);
  checkRadius = getParam<float>("checkRadius", 1.f);
  int kernelSize = getParam<int>("ksize", 64);

  // generate kernel with random sample distribution
  kernel.resize(kernelSize);
  randomVecs.resize(kernelSize);
  for (int i = 0; i < kernelSize; ++i) {
    kernel[i] = vec3f(-1.0f + 2 * (rand() % 1000) / 1000.0,
        -1.0f + 2 * (rand() % 1000) / 1000.0,
        (rand() % 1000) / 1000.0);
    float scale = float(i) / float(kernelSize);
    scale = 0.1 + 0.9 * scale * scale;
    kernel[i] *= scale;

    randomVecs[i].x = -1.0f + 2 * (rand() % 1000) / 1000.0;
    randomVecs[i].y = -1.0f + 2 * (rand() % 1000) / 1000.0;
    randomVecs[i].z = -1.0f + 2 * (rand() % 1000) / 1000.0;
    randomVecs[i] = normalize(randomVecs[i]);
  }
}

LiveSSAOFrameOp::LiveSSAOFrameOp(FrameBufferView &_fbView,
    void *ispcEquiv,
    float ssaoStrength,
    float radius,
    float checkRadius,
    std::vector<vec3f> kernel,
    std::vector<vec3f> randomVecs)
    : LiveFrameOp(_fbView),
      ispcEquiv(ispcEquiv),
      ssaoStrength(ssaoStrength),
      radius(radius),
      checkRadius(checkRadius),
      kernel(kernel),
      randomVecs(randomVecs)
{}

template <typename T>
void LiveSSAOFrameOp::applySSAO(
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

void LiveSSAOFrameOp::process(const Camera *camera)
{
  if (fbView.colorBufferFormat == OSP_FB_RGBA8
      || fbView.colorBufferFormat == OSP_FB_SRGBA) {
    // TODO: For SRGBA we actually need to convert to linear before filtering
    applySSAO(fbView, static_cast<uint8_t *>(fbView.colorBuffer), camera);
  } else {
    applySSAO(fbView, static_cast<float *>(fbView.colorBuffer), camera);
  }
}

OSP_REGISTER_IMAGE_OP(SSAOFrameOp, ssao);

} // namespace ospray
