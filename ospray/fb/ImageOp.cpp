// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageOp.h"

namespace ospray {

// ImageOp definitions ////////////////////////////////////////////////////////

ImageOp *ImageOp::createImageOp(const char *type, api::Device &device)
{
  return createInstance(type, device);
}

ImageOp::ImageOp()
{
  managedObjectType = OSP_IMAGE_OPERATION;
}

std::string ImageOp::toString() const
{
  return "ospray::ImageOp(base class)";
}

OSPTYPEFOR_DEFINITION(ImageOp *);

} // namespace ospray
