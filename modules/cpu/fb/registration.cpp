// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library
// so we have to include it with import define so the table will be imported
// from 'ospray' library
#define OBJECTFACTORY_IMPORT
#include "common/ObjectFactory.h"

#include "frame_ops/Blur.h"
#include "frame_ops/Debug.h"
#include "frame_ops/ToneMapper.h"

namespace ospray {

void registerAllImageOps()
{
  ImageOp::registerType<BlurFrameOp>("blur");
  ImageOp::registerType<DebugFrameOp>("debug");
  ImageOp::registerType<ToneMapperFrameOp>("tonemapper");
}

} // namespace ospray
