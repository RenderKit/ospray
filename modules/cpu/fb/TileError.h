// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "common/OSPCommon.h"
#include "rkcommon/containers/AlignedVector.h"

namespace ospray {

/* manages error per tile and adaptive regions, for variance estimation
  Implementation based on Dammertz et al., "A Hierarchical Automatic Stopping
  Condition for Monte Carlo Global Illumination", WSCG 2010 */
class OSPRAY_SDK_INTERFACE TileError
{
 public:
  TileError(const vec2i &numTiles);

  void clear();

  float operator[](const vec2i &tile) const;

  void update(const vec2i &tile, const float error);

  float refine(const float errorThreshold);

 protected:
  vec2i numTiles;
  int tiles;
  // holds error per tile
  containers::AlignedVector<float> tileErrorBuffer;
  // image regions (in #tiles) which do not yet estimate the error on
  // per-tile base
  std::vector<box2i> errorRegion;
};
} // namespace ospray
