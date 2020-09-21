// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unordered_map>

#include "camera/Camera.h"
#include "common/MPICommon.h"
#include "common/World.h"
#include "embree3/rtcore.h"
#include "rkcommon/math/box.h"

namespace ospray {
namespace mpi {

// The bounds of a region projected onto the image plane
struct RegionScreenBounds
{
  rkcommon::math::box2f bounds;
  // The max-depth of the box, for sorting the compositing order
  float depth = -std::numeric_limits<float>::infinity();

  // Extend the screen-space bounds to include p.xy
  // TODO WILL depth is treated separately by just getting the projection
  // along the camera view axis. So either we need the camera here, or
  // should just take the z val to only tell us if we should fill the whole
  // window.
  void extend(const rkcommon::math::vec3f &p);
};

/* A region is defined by its bounds and an ID, which allows us to group
 * ranks with the same region and switch to do image-parallel rendering
 */
struct Region
{
  rkcommon::math::box3f bounds;
  int id = -1;

  Region() = default;
  Region(const rkcommon::math::box3f &bounds, int id);

  RegionScreenBounds project(const Camera *camera) const;

  // Compiler can't find these when I define them outside in the public
  // namespace!????
  bool operator==(const Region &b) const;
  bool operator<(const Region &b) const;
};

/* The distributed world provides the renderer with the view of the local
 * and distributed objects in the entire scene across the cluster. For
 * remote objects we only store their bounds and the ID of the rank which
 * owns them. A region is some "clip box" within the world, which contains
 * some mix of volumes and geometries to be rendered, with rays clipped
 * against the bounds of this box.
 */
struct DistributedWorld : public World
{
  DistributedWorld();
  virtual ~DistributedWorld() override = default;

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
  std::vector<Region> allRegions;
  std::vector<size_t> myRegionIds;
  // The ranks which own each region
  std::unordered_map<int, std::set<size_t>> regionOwners;
};
} // namespace mpi
} // namespace ospray

std::ostream &operator<<(std::ostream &os, const ospray::mpi::Region &r);
