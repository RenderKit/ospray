// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageOp.h"

namespace ospray {

// ImageOp definitions ////////////////////////////////////////////////////////

ImageOp *ImageOp::createImageOp(const char *type, devicert::Device &device)
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

// explicitly create a key function to merge typeinfo from dynamically loaded
// modules (to reliably dynamic_cast<> for the denoiser)
// see https://github.com/android/ndk/issues/533#issuecomment-335977747
FrameOpInterface::~FrameOpInterface() {};

OSPTYPEFOR_DEFINITION(ImageOp *);

} // namespace ospray
