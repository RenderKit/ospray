// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unordered_map>

#include "camera/Camera.h"
#include "common/MPICommon.h"
#include "common/World.h"
#include "embree3/rtcore.h"
#include "geometry/Boxes.h"
#include "rkcommon/math/box.h"
// ispc shared
#include "DistributedWorldShared.h"

namespace ospray {
namespace mpi {

/* The distributed world provides the renderer with the view of the local
 * and distributed objects in the entire scene across the cluster. For
 * remote objects we only store their bounds and the ID of the rank which
 * owns them. A region is some "clip box" within the world, which contains
 * some mix of volumes and geometries to be rendered, with rays clipped
 * against the bounds of this box.
 */
struct DistributedWorld : public AddStructShared<World, ispc::DistributedWorld>
{
  DistributedWorld();
  ~DistributedWorld() override;

  box3f getBounds() const override;

  virtual std::string toString() const override;

  // commit synchronizes the distributed models between processes
  // so that ranks know how many tiles to expect for sort-last
  // compositing.
  virtual void commit() override;

 private:
  /* Perform the region bounds exchange among the ranks to build the
   * distributed view of the world
   */
  void exchangeRegions();

  // The communicator to use for collectives in the world
  mpicommon::Group mpiGroup;

 public:
  Ref<const DataT<box3f>> localRegions;
  std::vector<rkcommon::math::box3f> myRegions;
  // The global list of unique regions across all nodes, (including this
  // one), sorted by region id.
  std::vector<box3f> allRegions;
  std::vector<size_t> myRegionIds;
  // The ranks which own each region
  std::unordered_map<int, std::set<size_t>> regionOwners;

  Ref<Boxes> regionGeometry;
  RTCScene regionScene = nullptr;
};
} // namespace mpi
} // namespace ospray
