// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>
#include "../fb/TileOperation.h"
#include "common/MPICommon.h"
#include "rkcommon/containers/AlignedVector.h"

namespace ospray {
struct WriteMultipleTileOperation : public TileOperation
{
  void attach(DistributedFrameBuffer *dfb) override;

  std::shared_ptr<LiveTileOperation> makeTile(DistributedFrameBuffer *dfb,
      const vec2i &tileBegin,
      size_t tileID,
      size_t ownerID) override;

  std::string toString() const override;

  void syncTileInstances();

  rkcommon::containers::AlignedVector<uint32_t> tileInstances;
  mpicommon::Group mpiGroup;
};
} // namespace ospray
