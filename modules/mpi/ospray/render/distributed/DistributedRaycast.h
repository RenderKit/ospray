// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fstream>
#include <memory>
// ospray
#include "../../common/DistributedWorld.h"
#include "DistributedRenderer.h"
#include "camera/PerspectiveCamera.h"
#include "common/MPICommon.h"

namespace ospray {
namespace mpi {

/* The distributed raycast renderer supports rendering distributed
 * geometry and volume data, assuming that the data distribution is suitable
 * for sort-last compositing. Specifically, the data must be organized
 * among nodes such that each nodes region is convex and disjoint from the
 * others. The renderer uses the 'regions' data set on the distributed world
 * from the MPIDistributedDevice to determine the number of tiles to
 * render and expect for compositing.
 *
 * Also see apps/ospRandSciVisTest.cpp and apps/ospRandSphereTest.cpp for
 * example usage.
 */
struct DistributedRaycastRenderer : public DistributedRenderer
{
  DistributedRaycastRenderer();
  virtual ~DistributedRaycastRenderer() override;

  void commit() override;

  std::string toString() const override;

  std::shared_ptr<TileOperation> tileOperation() override;

 private:
  // The communicator to use for collectives in the renderer
  mpicommon::Group mpiGroup;

  std::unique_ptr<std::ofstream> statsLog;
};

} // namespace mpi
} // namespace ospray
