// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "AMRAccel.h"

namespace ospray {
  namespace amr {

    /*! constructor that constructs the actual accel from the amr data */
    AMRAccel::AMRAccel(const AMRData &input)
    {
      box3f bounds = empty;
      std::vector<const AMRData::Brick *> brickVec;
      for (auto &b : input.brick) {
        brickVec.push_back(&b);
        bounds.extend(b.worldBounds);
      }
      this->worldBounds = bounds;

      for (auto &b : brickVec) {
        if (b->level >= static_cast<int>(level.size()))
          level.resize(b->level+1);
        level[b->level].level = b->level;
        level[b->level].cellWidth = b->cellWidth;
        level[b->level].halfCellWidth = 0.5f*b->cellWidth;
        level[b->level].rcpCellWidth = 1.f/b->cellWidth;
      }

      node.resize(1);
      buildRec(0, bounds, brickVec);
    }

    /*! destructor that frees all allocated memory */
    AMRAccel::~AMRAccel()
    {
      for (auto &l : leaf)
        delete [] l.brickList;

      leaf.clear();
      node.clear();
    }

    void AMRAccel::makeLeaf(index_t nodeID,
                            const box3f &bounds,
                            const std::vector<const AMRData::Brick *> &brick)
    {
      node[nodeID].dim = 3;
      node[nodeID].ofs = this->leaf.size(); //item.size();
      node[nodeID].numItems = brick.size();

      AMRAccel::Leaf newLeaf;
      newLeaf.bounds = bounds;
      newLeaf.brickList = new const AMRData::Brick *[brick.size()+1];

      // create leaf list, and sort it
      std::copy(brick.begin(),brick.end(),newLeaf.brickList);
      std::sort(newLeaf.brickList,newLeaf.brickList+brick.size(),
                [&](const AMRData::Brick *a, const AMRData::Brick *b){
                  return a->level > b->level;
                });

      newLeaf.brickList[brick.size()] = nullptr;
      this->leaf.push_back(newLeaf);
    }

    void AMRAccel::makeInner(index_t nodeID, int dim, float pos, int childID)
    {
      node[nodeID].dim = dim;
      node[nodeID].pos = pos;
      node[nodeID].ofs = childID;
    }

    void AMRAccel::buildRec(int nodeID,
                            const box3f &bounds,
                            std::vector<const AMRData::Brick *> &brick)
    {
      std::set<float> possibleSplits[3];
      for (const auto &b : brick) {
        const box3f clipped = intersectionOf(bounds, b->worldBounds);
        assert(clipped.lower.x != clipped.upper.x);
        assert(clipped.lower.y != clipped.upper.y);
        assert(clipped.lower.z != clipped.upper.z);
        if (clipped.lower.x != bounds.lower.x)
          possibleSplits[0].insert(clipped.lower.x);
        if (clipped.upper.x != bounds.upper.x)
          possibleSplits[0].insert(clipped.upper.x);
        if (clipped.lower.y != bounds.lower.y)
          possibleSplits[1].insert(clipped.lower.y);
        if (clipped.upper.y != bounds.upper.y)
          possibleSplits[1].insert(clipped.upper.y);
        if (clipped.lower.z != bounds.lower.z)
          possibleSplits[2].insert(clipped.lower.z);
        if (clipped.upper.z != bounds.upper.z)
          possibleSplits[2].insert(clipped.upper.z);
      }

      int bestDim = -1;
      vec3f width = bounds.size();
      for (int dim=0;dim<3;dim++) {
        if (possibleSplits[dim].empty()) continue;
        if (bestDim == -1 || (width[dim] > width[bestDim]))
          bestDim = dim;
      }

      if (bestDim == -1) {
        // no split dim - make a leaf

        // note that by construction the last brick must be the onoe
        // we're looking for (all on a lower level must be earlier in
        // the list)

        makeLeaf(nodeID,bounds,brick);
      } else {
        float bestPos = std::numeric_limits<float>::infinity();
        float mid = bounds.center()[bestDim];
        for (const auto &split : possibleSplits[bestDim]) {
          if (fabsf(split - mid) < fabsf(bestPos-mid))
            bestPos = split;
        }
        box3f lBounds = bounds;
        box3f rBounds = bounds;

        switch(bestDim) {
        case 0:
          lBounds.upper.x = bestPos;
          rBounds.lower.x = bestPos;
          break;
        case 1:
          lBounds.upper.y = bestPos;
          rBounds.lower.y = bestPos;
          break;
        case 2:
          lBounds.upper.z = bestPos;
          rBounds.lower.z = bestPos;
          break;
        }

        std::vector<const AMRData::Brick *> l, r;
        float sum_lo = 0.f, sum_hi = 0.f;
        for (const auto &b : brick) {
          const box3f wb = intersectionOf(b->worldBounds,bounds);
          if (wb.empty())
            throw std::runtime_error("empty box!?");
          sum_lo += wb.lower[bestDim];
          sum_hi += wb.upper[bestDim];
          if (wb.lower[bestDim] >= bestPos) {
            r.push_back(b);
          } else if (wb.upper[bestDim] <= bestPos) {
            l.push_back(b);
          } else {
            r.push_back(b);
            l.push_back(b);
          }
        }
        if (l.empty() || r.empty()) {
          /* this here "should" never happen since the root level is
             always completely covered. if we do reach this code we
             have found a spatial region that doesn't contain *any*
             brick, so we can be pretty sure that "something" is
             missing :-/ */
          std::cerr << "ERROR: found non overlapped node in AMR structure\n";
          PRINT(bounds);
          PRINT(bestPos);
          PRINT(bestDim);
          PRINT(brick.size());
        }
        assert(!(l.empty() || r.empty()));

        int newNodeID = node.size();
        makeInner(nodeID,bestDim,bestPos,newNodeID);

        node.push_back(AMRAccel::Node());
        node.push_back(AMRAccel::Node());
        brick.clear();

        buildRec(newNodeID+0,lBounds,l);
        buildRec(newNodeID+1,rBounds,r);
      }
    }

  } // ::ospray::amr
} // ::ospray
