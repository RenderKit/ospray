// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageOp.h"
#include "fb/FrameBuffer.h"

namespace ospray {

// ImageOp definitions ////////////////////////////////////////////////////////

LiveImageOp::LiveImageOp(FrameBufferView &_fbView) : fbView(_fbView) {}

ImageOp::ImageOp()
{
  managedObjectType = OSP_IMAGE_OPERATION;
}

std::string ImageOp::toString() const
{
  return "ospray::ImageOp(base class)";
}

LivePixelOp::LivePixelOp(FrameBufferView &_fbView)
    : AddStructShared(
        _fbView.originalFB->getISPCDevice().getIspcrtDevice(), _fbView)
{}

LiveFrameOp::LiveFrameOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

OSPTYPEFOR_DEFINITION(ImageOp *);

} // namespace ospray
