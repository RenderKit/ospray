// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "frame_ops/Blur.h"
#include "frame_ops/Debug.h"
#include "frame_ops/Depth.h"
#include "frame_ops/SSAO.h"
#include "tile_ops/SaveTiles.h"
#include "tile_ops/ToneMapper.h"

namespace ospray {

void registerAllImageOps()
{
  ImageOp::registerType<BlurFrameOp>("blur");
  ImageOp::registerType<DebugFrameOp>("debug");
  ImageOp::registerType<DepthFrameOp>("depth");
  ImageOp::registerType<SSAOFrameOp>("ssao");
  ImageOp::registerType<SaveTiles>("save");
  ImageOp::registerType<ToneMapper>("tonemapper");
}

} // namespace ospray
