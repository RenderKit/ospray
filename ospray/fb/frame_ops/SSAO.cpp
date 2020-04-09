// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SSAO.h"

namespace ospray {

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

} // namespace ospray
