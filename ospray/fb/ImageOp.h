// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/ObjectFactory.h"

#include <string>

namespace ospray {

namespace api {
struct Device;
}

struct FrameBufferView;

// An instance of an image op that is actually attached to a framebuffer
struct OSPRAY_CORE_INTERFACE LiveImageOp
{
  virtual ~LiveImageOp() = default;
};

// Base abstraction for a "Image Op" to be performed for
// every image that gets written into a frame buffer.
// A ImageOp is basically a "hook" that allows to inject arbitrary
// code, such as postprocessing, filtering, blending, tone mapping,
// sending tiles to a display wall, etc.
struct OSPRAY_CORE_INTERFACE ImageOp
    : public ManagedObject,
      public ObjectFactory<ImageOp, api::Device &>
{
  static ImageOp *createImageOp(const char *type, api::Device &device);

  ImageOp();
  ~ImageOp() override = default;

  std::string toString() const override;
};

OSPTYPEFOR_SPECIALIZATION(ImageOp *, OSP_IMAGE_OPERATION);

struct Camera;
struct OSPRAY_CORE_INTERFACE LiveFrameOpInterface : public LiveImageOp
{
  ~LiveFrameOpInterface() override = default;

  virtual void process(void *waitEvent, const Camera *camera) = 0;
};

struct OSPRAY_CORE_INTERFACE FrameOpInterface : public ImageOp
{
  ~FrameOpInterface() override; // no =default!

  // Attach an image op to an existing framebuffer. Use this
  // to pass the params from the API to the instance of the image op
  // which will actually be run on the framebuffer view or tiles of the
  // framebuffer passed
  virtual std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) = 0;
};

} // namespace ospray
