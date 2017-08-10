// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#undef NDEBUG

#include "MinMaxBVH2.h"

// num prims that _force_ a leaf; undef to revert to sah termination criterion
//#define LEAF_THRESHOLD 2

static int leafCount = 0;

namespace ospray {
  using std::cout;
  using std::endl;

  __forceinline float my_safeArea(const box3f &b)
  {
    vec3f size = b.upper - b.lower;
    float f    = size.x * size.y + size.x * size.z + size.y * size.z;
    if (fabs(f) < 1e-20) {
      return 1e-20;
    }
    return f;
  }
  __forceinline float my_safeArea(const box4f &b)
  {
    vec4f size = b.upper - b.lower;
    float f    = size.x * size.y + size.x * size.z + size.y * size.z;
    if (fabs(f) < 1e-20) {
      return 1e-20;
    }
    return f;
  }

  void MinMaxBVH::buildRec(const size_t nodeID,
                           const box4f *const primBounds,
                           /*! tmp primid array, for non-inplace partition */
                           int64 *tmp_primID,
                           const size_t begin,
                           const size_t end)
  {
    bool dbg = ((end - begin) > 100000);
    if (dbg) {
      cout << "building minmaxbvh " << begin << " " << end << endl;
      PRINT(nodeID);
    }
    box4f bounds4     = empty;
    box4f centBounds4 = empty;
    float ctr[4];
    for (size_t i = begin; i < end; i++) {
      bounds4.extend(primBounds[primID[i]]);
      centBounds4.extend(center(primBounds[primID[i]]));
    }
    if (nodeID == 0) {
      overallBounds = bounds4;
    }

    const box3f centBounds((vec3f &)centBounds4.lower,
                           (vec3f &)centBounds4.upper);
    node[nodeID].lower = bounds4.lower;
    node[nodeID].upper = bounds4.upper;
    if (dbg) {
      cout << "bounds " << nodeID << " " << bounds4 << endl;
    }
    size_t dim        = arg_max(centBounds.size());
    (vec3f &)ctr      = center(centBounds);
    float pos         = ctr[dim];
    float costNoSplit = 1 + (end - begin);

    size_t l      = begin;
    size_t r      = end;
    box4f lBounds = empty;
    box4f rBounds = empty;
    for (int i = begin; i < end; i++) {
      (vec4f &)ctr = center(primBounds[primID[i]]);
      if (ctr[dim] < pos) {
        tmp_primID[l++] = primID[i];
        lBounds.extend(primBounds[primID[i]]);
      } else {
        tmp_primID[--r] = primID[i];
        rBounds.extend(primBounds[primID[i]]);
      }
    }
    for (int i = begin; i < end; i++) {
      primID[i] = tmp_primID[i];
    }

    if (dbg) {
      PRINT(l);
      PRINT(r);
      PRINT(begin);
      PRINT(end);
    }

    float costIfSplit =
        1 +
        (1.f / my_safeArea(bounds4)) * (my_safeArea(lBounds) * (l - begin) +

                                        my_safeArea(rBounds) * (end - l));
    if (dbg) {
      PRINT(costIfSplit);
      PRINT(costNoSplit);
      PRINT(lBounds);
      PRINT(rBounds);
    }
    if (
#ifdef LEAF_THRESHOLD
        (end - begin) <= LEAF_THRESHOLD ||
#endif
        ((costIfSplit >= costNoSplit) && ((end - begin) <= 7)) ||
        (l == begin) || (l == end)) {
      node[nodeID].childRef = (end - begin) + begin * sizeof(primID[0]);
      leafCount++;
    } else {
      assert(l > begin);
      assert(l < end);
      for (int i = begin; i < l; i++) {
        (vec4f &)ctr = center(primBounds[primID[i]]);
        assert(ctr[dim] < pos);
      }
      for (int i = l; i < end; i++) {
        (vec4f &)ctr = center(primBounds[primID[i]]);
        assert(ctr[dim] >= pos);
      }
      size_t childID        = node.size();
      node[nodeID].childRef = childID * sizeof(Node);
      node.push_back(Node());
      node.push_back(Node());
      buildRec(childID + 0, primBounds, tmp_primID, begin, l);
      buildRec(childID + 1, primBounds, tmp_primID, l, end);
    }
  }

  void
  MinMaxBVH::build(/*! one bounding box per primitive. The attribute value
                                             is in the 'w' component */
                   const box4f *const primBounds,
                   /*! primitive references; each entry refers to one
                       primitive, but the BVH or builder itself will _NOT_
                       specify what exactly one such 64-bit value stands for
                       (ie, it mmay be IDs, but does not have to. The BVH
                       will copy this array; the app can free after this
                       call*/
                   const int64 *const primRefs,
                   /*! number of primitives */
                   const size_t numPrims)
  {
    if (!this->node.empty()) {
      std::cout << "*REBUILDING* MinMaxBVH2!?" << std::endl;
    }
    this->primID.resize(numPrims);
    std::copy(primRefs, primRefs + numPrims, primID.begin());
    this->node.clear();
    this->node.push_back(Node());
    this->node.push_back(Node());
    /*! tmp primid array, for non-inplace partition */
    int64 *tmp_primID = new int64[numPrims];

    buildRec(0, primBounds, tmp_primID, 0, numPrims);
    this->node.resize(this->node.size());
    delete[] tmp_primID;

    rootRef = node[0].childRef;

    PRINT(leafCount);
    calcAndPrintOverlap();
  }

  float area(const MinMaxBVH::Node &node)
  {
    vec4f size = node.upper - node.lower;
    float f    = size.x * size.y + size.x * size.z + size.y * size.z;
    if (fabs(f) < 1e-20) {
      return 1e-20;
    }
    return f;
  }

  void MinMaxBVH::calcAndPrintOverlap() const
  {
    float overlapSum  = 0.f;
    int familyCount   = 1;
    int overlapsNear2 = 0;
    float near2Bound  = 1.80f;
    // Root is a special case...
    overlapSum += (area(node[2]) + area(node[3])) / area(node[0]);

    for (size_t i = 2; i < node.size(); i++) {
      int childRef = node[i].childRef;
      if ((childRef & 0x7) > 0) {
        continue;
      }
      int childIdx = (childRef & ~(7LL)) / sizeof(Node);
      familyCount++;
      float currentOverlap =
          (area(node[childIdx]) + area(node[childIdx + 1])) / area(node[i]);
      overlapSum += currentOverlap;
      if (currentOverlap > near2Bound) {
        overlapsNear2++;
      }
    }

    std::cout << "Total node families " << familyCount << std::endl;
    std::cout << "Sum of all overlaps " << overlapSum << std::endl;
    std::cout << "Average overlap: " << overlapSum / familyCount << std::endl;
    std::cout << "In " << overlapsNear2
              << " families there was an overlap greater than " << near2Bound
              << std::endl;
  }
}
