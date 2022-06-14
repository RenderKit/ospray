// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.h"
#include <algorithm>
#include <iterator>
#include "MPICommon.h"
#include "Messaging.h"
#include "ISPCDevice.h"
#include "common/Data.h"

namespace ospray {
namespace mpi {

using namespace rkcommon;

DistributedWorld::DistributedWorld() : mpiGroup(mpicommon::worker.dup())
{
  managedObjectType = OSP_WORLD;
}

DistributedWorld::~DistributedWorld()
{
  MPI_Comm_free(&mpiGroup.comm);
}

box3f DistributedWorld::getBounds() const
{
  box3f bounds;
  for (const auto &b : allRegions) {
    bounds.extend(b);
  }
  return bounds;
}

std::string DistributedWorld::toString() const
{
  return "ospray::mpi::DistributedWorld";
}

void DistributedWorld::commit()
{
  World::commit();

  allRegions.clear();
  myRegions.clear();
  myRegionIds.clear();
  regionOwners.clear();

  localRegions = getParamDataT<box3f>("region");
  if (localRegions) {
    std::copy(localRegions->begin(),
        localRegions->end(),
        std::back_inserter(myRegions));
  } else {
    // Assume we're going to treat everything on this node as a one region,
    // either for data-parallel rendering or to switch to replicated
    // rendering
    box3f localBounds;
    if (getSh()->super.embreeSceneHandleGeometries) {
      box4f b;
      rtcGetSceneBounds(
          getSh()->super.embreeSceneHandleGeometries, (RTCBounds *)&b);
      localBounds.extend(box3f(vec3f(b.lower.x, b.lower.y, b.lower.z),
          vec3f(b.upper.x, b.upper.y, b.upper.z)));
    }

    if (getSh()->super.embreeSceneHandleVolumes) {
      box4f b;
      rtcGetSceneBounds(
          getSh()->super.embreeSceneHandleVolumes, (RTCBounds *)&b);
      localBounds.extend(box3f(vec3f(b.lower.x, b.lower.y, b.lower.z),
          vec3f(b.upper.x, b.upper.y, b.upper.z)));
    }
    myRegions.push_back(localBounds);
  }

  // Figure out the unique regions on this node which we can send
  // to the others to build the distributed world
  std::sort(
      myRegions.begin(), myRegions.end(), [](const box3f &a, const box3f &b) {
        return a.lower < b.lower || (a.lower == b.lower && a.upper < b.upper);
      });
  auto last = std::unique(myRegions.begin(), myRegions.end());
  myRegions.erase(last, myRegions.end());

  exchangeRegions();

  if (regionScene) {
    rtcReleaseScene(regionScene);
    regionScene = nullptr;
  }

  if (allRegions.size() > 0) {
    // Setup the boxes geometry which we'll use to leverage Embree for
    // accurately determining region visibility
    Data *allRegionsData = new Data(allRegions.data(),
        OSP_BOX3F,
        vec3ul(allRegions.size(), 1, 1),
        vec3l(0));
    regionGeometry = new Boxes();
    regionGeometry->setParam("box", (ManagedObject *)allRegionsData);
    regionGeometry->setDevice(embreeDevice);
    regionGeometry->commit();

    regionScene = rtcNewScene(embreeDevice);
    rtcAttachGeometry(regionScene, regionGeometry->getEmbreeGeometry());
    rtcSetSceneFlags(regionScene, RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION);
    rtcCommitScene(regionScene);

    regionGeometry->refDec();
    allRegionsData->refDec();
  }

  getSh()->regions = allRegions.data();
  getSh()->numRegions = allRegions.size();
  getSh()->regionScene = regionScene;
}

void DistributedWorld::exchangeRegions()
{
  // Exchange regions between the ranks in world to find which other
  // ranks may be sharing a region with this one, and the other regions
  // to expect to be rendered for each tile
  for (int i = 0; i < mpiGroup.size; ++i) {
    if (i == mpiGroup.rank) {
      int nRegions = myRegions.size();

      auto sizeBcast =
          mpicommon::bcast(&nRegions, 1, MPI_INT, i, mpiGroup.comm);

      int nBytes = nRegions * sizeof(box3f);
      auto regionBcast = mpicommon::bcast(
          myRegions.data(), nBytes, MPI_BYTE, i, mpiGroup.comm);

      for (const auto &b : myRegions) {
        auto fnd = std::find_if(allRegions.begin(),
            allRegions.end(),
            [&](const box3f &r) { return r == b; });
        int id = -1;
        if (fnd == allRegions.end()) {
          id = allRegions.size();
          allRegions.push_back(b);
        } else {
          id = std::distance(allRegions.begin(), fnd);
        }
        regionOwners[id].insert(i);
        myRegionIds.push_back(id);
      }

      // As we recv/send regions see if they're unique or not

      sizeBcast.wait();
      regionBcast.wait();

    } else {
      int nRegions = 0;
      mpicommon::bcast(&nRegions, 1, MPI_INT, i, mpiGroup.comm).wait();

      std::vector<box3f> recv;
      recv.resize(nRegions);
      int nBytes = nRegions * sizeof(box3f);
      mpicommon::bcast(recv.data(), nBytes, MPI_BYTE, i, mpiGroup.comm).wait();

      for (const auto &b : recv) {
        auto fnd = std::find_if(allRegions.begin(),
            allRegions.end(),
            [&](const box3f &r) { return r == b; });
        int id = -1;
        if (fnd == allRegions.end()) {
          id = allRegions.size();
          allRegions.push_back(b);
        } else {
          id = std::distance(allRegions.begin(), fnd);
        }
        regionOwners[id].insert(i);
      }
    }
  }

  if (logLevel() >= OSP_LOG_DEBUG) {
    StatusMsgStream debugLog;
    for (int i = 0; i < mpiGroup.size; ++i) {
      if (i == mpiGroup.rank) {
        debugLog << "Rank " << mpiGroup.rank << ": All regions in world {\n";
        for (const auto &b : allRegions) {
          debugLog << "\t" << b << ",\n";
        }
        debugLog << "}\n\n";

        debugLog << "Ownership Information: {\n";
        for (const auto &r : regionOwners) {
          debugLog << "(" << r.first << ": [";
          for (const auto &i : r.second) {
            debugLog << i << ", ";
          }
          debugLog << "])\n";
        }
        debugLog << "\n";
      }
      mpicommon::barrier(mpiGroup.comm).wait();
    }
  }
}

} // namespace mpi
} // namespace ospray
