// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageOp.h"
#include "common/Util.h"
#include "fb/FrameBuffer.h"

namespace ospray {

static FactoryMap<ImageOp> g_imageOpMap;

// ImageOp definitions ////////////////////////////////////////////////////////

LiveImageOp::LiveImageOp(FrameBufferView &_fbView) : fbView(_fbView) {}

ImageOp *ImageOp::createInstance(const char *type)
{
  return createInstanceHelper(type, g_imageOpMap[type]);
}

void ImageOp::registerType(const char *type, FactoryFcn<ImageOp> f)
{
  g_imageOpMap[type] = f;
}

ImageOp::ImageOp()
{
  managedObjectType = OSP_IMAGE_OPERATION;
}

std::string ImageOp::toString() const
{
  return "ospray::ImageOp(base class)";
}

LivePixelOp::LivePixelOp(FrameBufferView &_fbView) : AddStructShared(_fbView) {}

LiveFrameOp::LiveFrameOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

OSPTYPEFOR_DEFINITION(ImageOp *);

} // namespace ospray
