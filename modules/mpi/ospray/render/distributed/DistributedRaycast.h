// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fstream>
#include <memory>
// ospray
#include "../../common/DistributedWorld.h"
#include "DistributedRenderer.h"
#include "camera/PerspectiveCamera.h"
#include "common/MPICommon.h"
// ispc shared
#include "DistributedRaycastShared.h"

namespace ospray {
namespace mpi {

/* The distributed raycast renderer supports rendering distributed
 * geometry and volume data, assuming that the data distribution is suitable
 * for sort-last compositing. Specifically, the data must be organized
 * among nodes such that each nodes region is convex and disjoint from the
 * others. The renderer uses the 'regions' data set on the distributed world
 * from the MPIDistributedDevice to determine the number of tiles to
 * render and expect for compositing.
 */
struct DistributedRaycastRenderer : public AddStructShared<DistributedRenderer,
                                        ispc::DistributedRaycastRenderer>
{
  DistributedRaycastRenderer(api::ISPCDevice &device);
  virtual ~DistributedRaycastRenderer() override;

  void commit() override;

  std::string toString() const override;

  std::shared_ptr<TileOperation> tileOperation() override;

#ifndef OSPRAY_TARGET_SYCL
  void renderRegionTasks(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const override;
#else
  void renderRegionTasks(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const override;
#endif

 private:
  // The communicator to use for collectives in the renderer
  mpicommon::Group mpiGroup;

  std::unique_ptr<std::ofstream> statsLog;
};

} // namespace mpi
} // namespace ospray
