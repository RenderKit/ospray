// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "MultiDeviceObject.h"
#include "fb/LocalFB.h"

namespace ospray {
namespace api {

/* The MultiDeviceFrameBuffer stores a SparseFB for each subdevice containing
 * the tiles the subdevice will render, along with a rowmajorFb that will be
 * exposed to the application. After rendering, the sparse framebuffers are
 * untiled into the rowmajorFb to return the rendered image to the app
 */
struct MultiDeviceFrameBuffer : MultiDeviceObject
{
  Ref<LocalFrameBuffer> rowmajorFb = nullptr;
};

} // namespace api
} // namespace ospray
