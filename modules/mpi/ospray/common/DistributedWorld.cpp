// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.h"
#include <algorithm>
#include <iterator>
#include "DistributedWorld.h"
#include "MPICommon.h"
#include "Messaging.h"
#include "api/ISPCDevice.h"
#include "common/Data.h"
#include "common/DistributedWorld_ispc.h"

namespace ospray {
namespace mpi {

using namespace rkcommon;

void RegionScreenBounds::extend(const vec3f &p)
{
  if (p.z < 0) {
    bounds = box2f(vec2f(0), vec2f(1));
  } else {
    bounds.extend(vec2f(p.x * sign(p.z), p.y * sign(p.z)));
    bounds.lower.x = clamp(bounds.lower.x);
    bounds.upper.x = clamp(bounds.upper.x);
    bounds.lower.y = clamp(bounds.lower.y);
    bounds.upper.y = clamp(bounds.upper.y);
  }
}

Region::Region(const box3f &bounds, int id) : bounds(bounds), id(id) {}

RegionScreenBounds Region::project(const Camera *camera) const
{
  RegionScreenBounds screen;
  for (int k = 0; k < 2; ++k) {
    vec3f pt;
    pt.z = k == 0 ? bounds.lower.z : bounds.upper.z;

    for (int j = 0; j < 2; ++j) {
      pt.y = j == 0 ? bounds.lower.y : bounds.upper.y;

      for (int i = 0; i < 2; ++i) {
        pt.x = i == 0 ? bounds.lower.x : bounds.upper.x;

        ProjectedPoint proj = camera->projectPoint(pt);
        screen.extend(proj.screenPos);
        screen.depth = std::max(length(pt - camera->pos), screen.depth);
      }
    }
  }
  return screen;
}

bool Region::operator==(const Region &b) const
{
  // TODO: Do we want users to specify the ID explitly? Or should we just
  // assume that two objects with the same bounds have the same id?
  return id == b.id;
}

bool Region::operator<(const Region &b) const
{
  return id < b.id;
}

DistributedWorld::DistributedWorld() : mpiGroup(mpicommon::worker.dup())
{
  managedObjectType = OSP_WORLD;
  this->ispcEquivalent = ispc::DistributedWorld_create(this);
}

box3f DistributedWorld::getBounds() const
{
  box3f bounds;
  for (const auto &r : allRegions) {
    bounds.extend(r.bounds);
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

  myRegions.clear();

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
    if (embreeSceneHandleGeometries) {
      box4f b;
      rtcGetSceneBounds(embreeSceneHandleGeometries, (RTCBounds *)&b);
      localBounds.extend(box3f(vec3f(b.lower.x, b.lower.y, b.lower.z),
          vec3f(b.upper.x, b.upper.y, b.upper.z)));
    }

    if (embreeSceneHandleVolumes) {
      box4f b;
      rtcGetSceneBounds(embreeSceneHandleVolumes, (RTCBounds *)&b);
      localBounds.extend(box3f(vec3f(b.lower.x, b.lower.y, b.lower.z),
          vec3f(b.upper.x, b.upper.y, b.upper.z)));
    }
    myRegions.push_back(localBounds);
  }

  // Figure out the unique regions on this node which we can send
  // to the others to build the distributed world
  std::sort(
      myRegions.begin(), myRegions.end(), [](const box3f &a, const box3f &b) {
        std::less<vec3f> op;
        return op(a.lower, b.lower)
            || (a.lower == b.lower && op(a.upper, b.upper));
      });
  auto last = std::unique(myRegions.begin(), myRegions.end());
  myRegions.erase(last, myRegions.end());

  exchangeRegions();

  ispc::DistributedWorld_set(getIE(), allRegions.data(), allRegions.size());
}

void DistributedWorld::exchangeRegions()
{
  allRegions.clear();

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
            [&](const Region &r) { return r.bounds == b; });
        int id = -1;
        if (fnd == allRegions.end()) {
          id = allRegions.size();
          allRegions.push_back(Region(b, id));
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
            [&](const Region &r) { return r.bounds == b; });
        int id = -1;
        if (fnd == allRegions.end()) {
          id = allRegions.size();
          allRegions.push_back(Region(b, id));
        } else {
          id = std::distance(allRegions.begin(), fnd);
        }
        regionOwners[id].insert(i);
      }
    }
  }

#ifndef __APPLE__
  // TODO WILL: Remove this eventually? It may be useful for users to debug
  // their code when setting regions. Maybe fix build on Apple? Why did
  // it fail to compile?
  if (logLevel() >= OSP_LOG_DEBUG) {
    for (int i = 0; i < mpiGroup.size; ++i) {
      if (i == mpiGroup.rank) {
        postStatusMsg(OSP_LOG_DEBUG)
            << "Rank " << mpiGroup.rank << ": All regions in world {";
        for (const auto &b : allRegions) {
          postStatusMsg(OSP_LOG_DEBUG) << "\t" << b << ",";
        }
        postStatusMsg(OSP_LOG_DEBUG) << "}\n";

        postStatusMsg(OSP_LOG_DEBUG) << "Ownership Information: {";
        for (const auto &r : regionOwners) {
          postStatusMsg(OSP_LOG_DEBUG) << "(" << r.first << ": [";
          for (const auto &i : r.second) {
            postStatusMsg(OSP_LOG_DEBUG) << i << ", ";
          }
          postStatusMsg(OSP_LOG_DEBUG) << "])";
        }
        postStatusMsg(OSP_LOG_DEBUG) << "\n" << std::flush;
      }
      mpicommon::barrier(mpiGroup.comm).wait();
    }
  }
#endif
}

} // namespace mpi
} // namespace ospray

using namespace ospray::mpi;

std::ostream &operator<<(std::ostream &os, const Region &r)
{
  os << "Region { id = " << r.id << ", bounds = " << r.bounds << " }";
  return os;
}
