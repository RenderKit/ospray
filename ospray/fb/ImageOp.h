// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FrameBufferView.h"
#include "fb/Tile.h"

namespace ospray {

struct Camera;

/*! An instance of an image op which is actually attached to a framebuffer */
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
  virtual ~ImageOp() override = default;

  virtual std::string toString() const override;

  /*! Attach an image op to an existing framebuffer. Use this
   * to pass the params from the API to the instance of the image op
   * which will actually be run on the framebuffer view or tiles of the
   * framebuffer passed
   */
  virtual std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) = 0;

  static ImageOp *createInstance(const char *type);
};

OSPTYPEFOR_SPECIALIZATION(ImageOp *, OSP_IMAGE_OPERATION);

struct OSPRAY_SDK_INTERFACE TileOp : public ImageOp
{
  virtual ~TileOp() override = default;
};

struct OSPRAY_SDK_INTERFACE FrameOp : public ImageOp
{
  virtual ~FrameOp() override = default;
  virtual vec2i haloSize()
  {
    return vec2i(0);
  }
};

struct OSPRAY_SDK_INTERFACE LiveTileOp : public LiveImageOp
{
  LiveTileOp(FrameBufferView &fbView);
  virtual ~LiveTileOp() override = default;

  /*! called right after the tile got accumulated; i.e., the
    tile's RGBA values already contain the accu-buffer blended
    values (assuming an accubuffer exists), and this function
    defines how these pixels are being processed before written
    into the color buffer */
  virtual void process(Tile &) = 0;
};

struct OSPRAY_SDK_INTERFACE LiveFrameOp : public LiveImageOp
{
  LiveFrameOp(FrameBufferView &fbView);
  virtual ~LiveFrameOp() override = default;

  virtual void process(const Camera *camera) = 0;
};

/*! \brief registers a internal ospray::<ClassName> renderer under
    the externally accessible name "external_name"

    \internal This currently works by defining a extern "C" function
    with a given predefined name that creates a new instance of this
    imageop. By having this symbol in the shared lib ospray can
    later on always get a handle to this fct and create an instance
    of this imageop.
*/
#define OSP_REGISTER_IMAGE_OP(InternalClass, external_name)                    \
  OSP_REGISTER_OBJECT(ImageOp, image_operation, InternalClass, external_name)

} // namespace ospray
