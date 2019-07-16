// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <algorithm>
#include "common/Util.h"
#include "FrameBuffer.h"
#include "FrameOpExamples.h"
#include "FrameOpExamples_ispc.h"
#include "camera/PerspectiveCamera.h"

namespace ospray {

  std::unique_ptr<LiveImageOp> DebugFrameOp::attach(FrameBufferView &fbView)
  {
    if (!fbView.colorBuffer) {
      static WarnOnce warn(toString() + " requires color data but the "
                           "framebuffer does not have this channel.");
      throw std::runtime_error(toString() + " cannot be attached to framebuffer "
                               "which does not have color data");
    }
    return ospcommon::make_unique<LiveDebugFrameOp>(fbView);
  }

  std::string DebugFrameOp::toString() const
  {
    return "ospray::DebugFrameOp";
  }

  LiveDebugFrameOp::LiveDebugFrameOp(FrameBufferView &fbView)
    : LiveFrameOp(fbView)
  {}

  void LiveDebugFrameOp::process(const Camera *)
  {
    // DebugFrameOp just colors the whole frame with red
    tasking::parallel_for(fbView.viewDims.x * fbView.viewDims.y,
      [&](int i)
      {
        if (fbView.colorBufferFormat == OSP_FB_RGBA8
            || fbView.colorBufferFormat == OSP_FB_SRGBA)
        {
          uint8_t *pixel = static_cast<uint8_t*>(fbView.colorBuffer) + i * 4;
          pixel[0] = 255;
        } else {
          float *pixel = static_cast<float*>(fbView.colorBuffer) + i * 4;
          pixel[0] = 1.f;
        }
      });
  }

  std::unique_ptr<LiveImageOp> BlurFrameOp::attach(FrameBufferView &fbView)
  {
    if (!fbView.colorBuffer) {
      static WarnOnce warn(toString() + " requires color data but the "
                           "framebuffer does not have this channel.");
      throw std::runtime_error(toString() + " cannot be attached to framebuffer "
                               "which does not have color data");
    }
    if (fbView.colorBufferFormat == OSP_FB_RGBA8
        || fbView.colorBufferFormat == OSP_FB_SRGBA)
    {
      return ospcommon::make_unique<LiveBlurFrameOp<uint8_t>>(fbView);
    }
    return ospcommon::make_unique<LiveBlurFrameOp<float>>(fbView);
  }

  std::string BlurFrameOp::toString() const
  {
    return "ospray::BlurFrameOp";
  }

  std::unique_ptr<LiveImageOp> DepthFrameOp::attach(FrameBufferView &fbView)
  {
    if (!fbView.colorBuffer || !fbView.depthBuffer) {
      static WarnOnce warn(toString() + " requires color and depth data but "
                           "the framebuffer does not have these channels.");
      throw std::runtime_error(toString() + " requires color and depth but "
                               "the framebuffer does not have these channels");
    }
    return ospcommon::make_unique<LiveDepthFrameOp>(fbView);
  }

  std::string DepthFrameOp::toString() const
  {
    return "ospray::DepthFrameOp";
  }

  LiveDepthFrameOp::LiveDepthFrameOp(FrameBufferView &fbView)
    : LiveFrameOp(fbView)
  {}

  void LiveDepthFrameOp::process(const Camera *)
  {
    // First find the min/max depth range to normalize the image,
    // we don't use minmax_element here b/c we don't want inf to be
    // found as the max depth value
    const int numPixels = fbView.fbDims.x * fbView.fbDims.y;
    vec2f depthRange(std::numeric_limits<float>::infinity(),
                     -std::numeric_limits<float>::infinity());
    for (int i = 0; i < numPixels; ++i)
    {
      if (!std::isinf(fbView.depthBuffer[i]))
      {
        depthRange.x = std::min(depthRange.x, fbView.depthBuffer[i]);
        depthRange.y = std::max(depthRange.y, fbView.depthBuffer[i]);
      }
    }
    const float denom = 1.f / (depthRange.y - depthRange.x);

    tasking::parallel_for(numPixels,
      [&](int px) {
        float normalizedZ = 1.f;
        if (!std::isinf(fbView.depthBuffer[px]))
          normalizedZ = (fbView.depthBuffer[px] - depthRange.x) * denom;

        for (int c = 0; c < 3; ++c)
        {
          if (fbView.colorBufferFormat == OSP_FB_RGBA8
              || fbView.colorBufferFormat == OSP_FB_SRGBA)
          {
            uint8_t *cbuf = static_cast<uint8_t *>(fbView.colorBuffer);
            cbuf[px * 4 + c] = static_cast<uint8_t>(normalizedZ * 255.f);
          } else {
            float *cbuf = static_cast<float *>(fbView.colorBuffer);
            cbuf[px * 4 + c] = normalizedZ;
          }
        }
        if (fbView.albedoBuffer) {
          fbView.albedoBuffer[px] = vec3f(normalizedZ);
        }
      });
  }


  std::unique_ptr<LiveImageOp> SSAOFrameOp::attach(FrameBufferView &fbView)
  {
    if (!fbView.colorBuffer) {
      static WarnOnce warn(toString() + " requires color data but the "
                           "framebuffer does not have this channel.");
      throw std::runtime_error(toString() + " cannot be attached to framebuffer "
                               "which does not have color data");
    }

    if (!fbView.normalBuffer || !fbView.depthBuffer) {
      static WarnOnce warn(toString() + " requires normal and depth data but "
                           "the framebuffer does not have these channels.");
      throw std::runtime_error(toString() + " cannot be attached to framebuffer "
                               "which does not have normal or depth data");
    }

    void *ispcEquiv = ispc::LiveSSAOFrameOp_create();

    return ospcommon::make_unique<LiveSSAOFrameOp>( fbView, 
                                                    ispcEquiv, 
                                                    ssaoStrength,
                                                    radius,
                                                    checkRadius,
                                                    kernel,
                                                    randomVecs);
  }


  std::string SSAOFrameOp::toString() const
  {
    return "ospray::SSAOFrameOp";
  }

  void SSAOFrameOp::commit(){

    ssaoStrength    = getParam1f("strength", 1.f);
    windowSize      = getParam2f("windowSize",  vec2f(-1.f));
    radius          = getParam1f("radius", 0.3f);
    checkRadius     = getParam1f("checkRadius", 1.f);
    int kernelSize  = getParam1i("ksize", 64);

    // generate kernel with random sample distribution
    kernel.resize(kernelSize);
    randomVecs.resize(kernelSize);
    for (int i = 0; i < kernelSize; ++i) {
        kernel[i] = vec3f(
          -1.0f+2*(rand()%1000)/1000.0,
          -1.0f+2*(rand()%1000)/1000.0,
          (rand()%1000)/1000.0);
        float scale = float(i) / float(kernelSize);
        scale = 0.1 +0.9*scale*scale;
        kernel[i] *= scale;

        randomVecs[i].x = -1.0f+2*(rand()%1000)/1000.0;
        randomVecs[i].y = -1.0f+2*(rand()%1000)/1000.0;
        randomVecs[i].z = -1.0f+2*(rand()%1000)/1000.0;
        randomVecs[i] = normalize(randomVecs[i]);
    }
  }

  LiveSSAOFrameOp::LiveSSAOFrameOp( FrameBufferView &fbView,
                                    void* ispcEquiv,
                                    float ssaoStrength,
                                    float radius, 
                                    float checkRadius,
                                    std::vector<vec3f> kernel,
                                    std::vector<vec3f> randomVecs)
    : LiveFrameOp(fbView),
    ispcEquiv(ispcEquiv),
    ssaoStrength(ssaoStrength),
    radius(radius),
    checkRadius(checkRadius),
    kernel(kernel),
    randomVecs(randomVecs)
  {}


  template<typename T>
  void LiveSSAOFrameOp::applySSAO(FrameBufferView &fb, T *color, const Camera *cam)
  {
    if (cam->toString().compare("ospray::PerspectiveCamera"))
    {
      throw std::runtime_error(cam->toString() + " camera type not supported in SSAO "
                               "PerspectiveCamera only");
    }
    PerspectiveCamera* camera = (PerspectiveCamera*)(cam);

    // get camera 
    vec3f eyePos    = camera->pos;
    vec3f lookDir   = camera->dir;
    vec3f upDir     = camera->up;
    float nearClip  = camera->nearClip;

    // get cameraSpace matrix
    vec3f zaxis = normalize(-lookDir);
    vec3f xaxis = normalize(cross(upDir, zaxis));
    vec3f yaxis = cross(zaxis, xaxis);
    vec3f p_component = vec3f(-dot(eyePos, xaxis), -dot(eyePos, yaxis), -dot(eyePos, zaxis));
    LinearSpace3f cameraModelRot(xaxis, yaxis, zaxis);
    AffineSpace3f cameraSpace = (AffineSpace3f(cameraModelRot.transposed(), p_component));

    vec2f windowSize = fb.fbDims;
    vec2f pixelSize;
    // get screen pixel size in camera space
    float fovy = camera->fovy;
    pixelSize.y = nearClip *  2.f * tanf(deg2rad(0.5f * fovy))/windowSize.y;
    pixelSize.x = pixelSize.y;

    ispc::LiveSSAOFrameOp_set(ispcEquiv,
                              nearClip,
                              ssaoStrength,
                              &windowSize,
                              &pixelSize,
                              &cameraSpace,
                              &kernel[0],
                              &randomVecs[0]);

    float * depthBuffer = fb.depthBuffer;
    std::vector<float> occlusionBuffer(fb.fbDims.x * fb.fbDims.y, 0);
    tasking::parallel_for(fb.fbDims.x * fb.fbDims.y,
      [&](int pixelID)
      {
        if(std::isinf(depthBuffer[pixelID]))
          occlusionBuffer[pixelID] = -1;;
      }
    );

    int programcount = ispc::getProgramCount();
    tasking::parallel_for(fb.fbDims.x * fb.fbDims.y / programcount,
      [&](int programID)
    {
      ispc::LiveSSAOFrameOp_getOcclusion( ispcEquiv, 
                                          &fb, 
                                          &occlusionBuffer[0], 
                                          radius, 
                                          checkRadius, 
                                          kernel.size(), 
                                          programID);
    });

    ispc::LiveSSAOFrameOp_applyOcclusion( ispcEquiv, 
                                          &fb, 
                                          color, 
                                          &occlusionBuffer[0]);

  }

  void LiveSSAOFrameOp::process(const Camera *camera)
  {


    if (fbView.colorBufferFormat == OSP_FB_RGBA8
        || fbView.colorBufferFormat == OSP_FB_SRGBA)
    {
      // TODO: For SRGBA we actually need to convert to linear before filtering
      applySSAO(fbView, static_cast<uint8_t*>(fbView.colorBuffer), camera);
    } else {
      applySSAO(fbView, static_cast<float*>(fbView.colorBuffer), camera);
    }
  }


  OSP_REGISTER_IMAGE_OP(SSAOFrameOp, frame_ssao);
  OSP_REGISTER_IMAGE_OP(DebugFrameOp, frame_debug);
  OSP_REGISTER_IMAGE_OP(BlurFrameOp, frame_blur);
  OSP_REGISTER_IMAGE_OP(DepthFrameOp, frame_depth);
}

