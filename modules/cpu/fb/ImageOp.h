// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FrameBufferView.h"
#include "ImageOpShared.h"
#include "common/Util.h"
#include "fb/TileShared.h"

namespace ospray {

struct Camera;

// An instance of an image op that is actually attached to a framebuffer
struct OSPRAY_SDK_INTERFACE LiveImageOp
{
  FrameBufferView fbView;

  LiveImageOp(FrameBufferView &fbView);

  virtual ~LiveImageOp() = default;

  virtual void beginFrame() {}

  virtual void endFrame() {}
};

/*! \brief base abstraction for a "Image Op" to be performed for
    every image that gets written into a frame buffer.

    A ImageOp is basically a "hook" that allows to inject arbitrary
    code, such as postprocessing, filtering, blending, tone mapping,
    sending tiles to a display wall, etc.
*/
struct OSPRAY_SDK_INTERFACE ImageOp : public ManagedObject
{
  ImageOp();
  ~ImageOp() override = default;

  std::string toString() const override;

  /*! Attach an image op to an existing framebuffer. Use this
   * to pass the params from the API to the instance of the image op
   * which will actually be run on the framebuffer view or tiles of the
   * framebuffer passed
   */
  virtual std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) = 0;

  static ImageOp *createInstance(const char *type);
  template <typename T>
  static void registerType(const char *type);

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<ImageOp> f);
};

OSPTYPEFOR_SPECIALIZATION(ImageOp *, OSP_IMAGE_OPERATION);

struct OSPRAY_SDK_INTERFACE PixelOp : public ImageOp
{
  ~PixelOp() override = default;
};

struct OSPRAY_SDK_INTERFACE FrameOp : public ImageOp
{
  ~FrameOp() override = default;
  virtual vec2i haloSize()
  {
    return vec2i(0);
  }
};

struct OSPRAY_SDK_INTERFACE LivePixelOp
    : public AddStructShared<LiveImageOp, ispc::LivePixelOp>
{
  LivePixelOp(FrameBufferView &fbView);
  ~LivePixelOp() override = default;
};

struct OSPRAY_SDK_INTERFACE LiveFrameOp : public LiveImageOp
{
  LiveFrameOp(FrameBufferView &fbView);
  ~LiveFrameOp() override = default;

  virtual void process(const Camera *camera) = 0;
};

// Inlined definitions ////////////////////////////////////////////////////////

template <typename T>
inline void ImageOp::registerType(const char *type)
{
  registerTypeHelper<ImageOp, T>(type);
}

} // namespace ospray
