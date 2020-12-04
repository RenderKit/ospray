// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>
#include "../../fb/TileOperation.h"

namespace ospray {
struct AlphaCompositeTileOperation : public TileOperation
{
  std::shared_ptr<LiveTileOperation> makeTile(DistributedFrameBuffer *dfb,
      const vec2i &tileBegin,
      size_t tileID,
      size_t ownerID) override;

  std::string toString() const override;
};
} // namespace ospray
