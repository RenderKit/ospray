// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class ImageOperation
    : public ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>
{
 public:
  ImageOperation(const std::string &type);
  ImageOperation(const ImageOperation &copy);
  ImageOperation(OSPImageOperation existing = nullptr);
};

static_assert(sizeof(ImageOperation) == sizeof(OSPImageOperation),
    "cpp::ImageOperation can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline ImageOperation::ImageOperation(const std::string &type)
{
  ospObject = ospNewImageOperation(type.c_str());
}

inline ImageOperation::ImageOperation(const ImageOperation &copy)
    : ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>(copy.handle())
{
  ospRetain(copy.handle());
}

inline ImageOperation::ImageOperation(OSPImageOperation existing)
    : ManagedObject<OSPImageOperation, OSP_IMAGE_OPERATION>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::ImageOperation, OSP_IMAGE_OPERATION);

} // namespace ospray
