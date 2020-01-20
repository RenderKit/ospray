// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageOp.h"
#include "common/Util.h"
#include "fb/FrameBuffer.h"

namespace ospray {

LiveImageOp::LiveImageOp(FrameBufferView &_fbView) : fbView(_fbView) {}

ImageOp *ImageOp::createInstance(const char *type)
{
  return createInstanceHelper<ImageOp, OSP_IMAGE_OPERATION>(type);
}

ImageOp::ImageOp()
{
  managedObjectType = OSP_IMAGE_OPERATION;
}

std::string ImageOp::toString() const
{
  return "ospray::ImageOp(base class)";
}

LiveTileOp::LiveTileOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

LiveFrameOp::LiveFrameOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

OSPTYPEFOR_DEFINITION(ImageOp *);

} // namespace ospray
