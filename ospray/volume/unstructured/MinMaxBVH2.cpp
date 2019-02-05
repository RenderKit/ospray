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

#include "MinMaxBVH2.h"

// num prims that _force_ a leaf; undef to revert to sah termination criterion
//#define LEAF_THRESHOLD 2

namespace ospray {

  template <typename T, int SIZE>
  inline float safeArea(const box_t<T, SIZE> &b)
  {
    auto size = b.upper - b.lower;
    float f   = size.x * size.y + size.x * size.z + size.y * size.z;
    return std::max(std::fabs(f), 1e-20f);
  }

  void MinMaxBVH2::buildRec(const size_t nodeID,
                            const box4f *const primBounds,
                            /*! tmp primid array, for non-inplace partition */
                            int64 *tmp_primID,
                            const size_t begin,
                            const size_t end)
  {
    box4f bounds4     = empty;
    box4f centBounds4 = empty;

    union
    {
      vec3f as_vec3f;
      vec4f as_vec4f;
      float raw[4];
    } ctr;

    for (size_t i = begin; i < end; i++) {
      bounds4.extend(primBounds[primID[i]]);
      centBounds4.extend(center(primBounds[primID[i]]));
    }

    if (nodeID == 0)
      overallBounds = bounds4;

    node[nodeID].lower = bounds4.lower;
    node[nodeID].upper = bounds4.upper;

    const box3f centBounds((vec3f &)centBounds4.lower,
                           (vec3f &)centBounds4.upper);

    size_t dim        = arg_max(centBounds.size());
    ctr.as_vec3f      = center(centBounds);
    float pos         = ctr.raw[dim];
    float costNoSplit = 1 + (end - begin);

    size_t l      = begin;
    size_t r      = end;
    box4f lBounds = empty;
    box4f rBounds = empty;

    for (size_t i = begin; i < end; i++) {
      ctr.as_vec4f = center(primBounds[primID[i]]);

      if (ctr.raw[dim] < pos) {
        tmp_primID[l++] = primID[i];
        lBounds.extend(primBounds[primID[i]]);
      } else {
        tmp_primID[--r] = primID[i];
        rBounds.extend(primBounds[primID[i]]);
      }
    }

    for (size_t i = begin; i < end; i++) {
      primID[i] = tmp_primID[i];
    }

    float costIfSplit =
        1 +
        (1.f / safeArea(bounds4)) * (safeArea(lBounds) * (l - begin) +

                                     safeArea(rBounds) * (end - l));

    if (
#ifdef LEAF_THRESHOLD
        (end - begin) <= LEAF_THRESHOLD ||
#endif
        ((costIfSplit >= costNoSplit) && ((end - begin) <= 7)) ||
        (l == begin) || (l == end)) {
      node[nodeID].childRef = (end - begin) + begin * sizeof(primID[0]);
    } else {
      assert(l > begin);
      assert(l < end);
      for (size_t i = begin; i < l; i++) {
        ctr.as_vec4f = center(primBounds[primID[i]]);
        assert(ctr.raw[dim] < pos);
      }
      for (size_t i = l; i < end; i++) {
        ctr.as_vec4f = center(primBounds[primID[i]]);
        assert(ctr.raw[dim] >= pos);
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
  MinMaxBVH2::build(/*! one bounding box per primitive. The attribute value
                                             is in the 'w' component */
                    const box4f *const primBounds,
                    /*! primitive references; each entry refers to one
                        primitive, but the BVH or builder itself will _NOT_
                        specify what exactly one such 64-bit value stands for
                        (ie, it mmay be IDs, but does not have to. The BVH
                        will copy this array; the app can free after this
                        call*/
                    const int64 *const primRefs,
                    const size_t numPrims)
  {
    this->primID.resize(numPrims);
    std::copy(primRefs, primRefs + numPrims, primID.begin());

    this->node.clear();
    this->node.push_back(Node());
    this->node.push_back(Node());

    /*! tmp primid array, for non-inplace partition */
    std::vector<int64> tmp_primID(numPrims);

    buildRec(0, primBounds, tmp_primID.data(), 0, numPrims);
    this->node.resize(this->node.size());

    root = node[0].childRef;
  }

  const void *MinMaxBVH2::nodePtr() const
  {
    assert(!node.empty());
    return node.data();
  }

  const int64 *MinMaxBVH2::itemListPtr() const
  {
    assert(!primID.empty());
    return primID.data();
  }

  uint64 MinMaxBVH2::rootRef() const
  {
    return root;
  }

  const box4f &MinMaxBVH2::bounds() const
  {
    return overallBounds;
  }

}  // ::ospray
