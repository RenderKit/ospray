// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Camera : public ManagedObject<OSPCamera, OSP_CAMERA>
{
 public:
  Camera(const std::string &type);
  Camera(OSPCamera existing = nullptr);
};

static_assert(sizeof(Camera) == sizeof(OSPCamera),
    "cpp::Camera can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Camera::Camera(const std::string &type)
{
  ospObject = ospNewCamera(type.c_str());
}

inline Camera::Camera(OSPCamera existing)
    : ManagedObject<OSPCamera, OSP_CAMERA>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Camera, OSP_CAMERA);

} // namespace ospray
