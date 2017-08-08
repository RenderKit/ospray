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

#pragma once

// ospray d
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"

namespace ospray
{

/*! defines a (binary) BVH with some float min/max value per
    node. The BVH itself does not specify the primitive type it is
    used with, or what those min/max values represent. */
struct MinMaxBVH {
    /*! a node in a MinMaxBVH: a (4D-)bounding box, plus a child/leaf refence */
    struct Node : public box4f {
        uint64 childRef;
    };
    void buildRec(const size_t nodeID, const box4f *const primBounds,
                  int64 *tmp_primID,
                  const size_t begin, const size_t end);
    void build(/*! one bounding box per primitive. The attribute value
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
               const size_t numPrims);
    /*! to allow passing this pointer to ISCP: */
    const void *getNodePtr() const
    {
        assert(!node.empty());
        return &node[0];
    };
    /*! to allow passing this pointer to ISCP: */
    const int64 *getItemListPtr() const
    {
        assert(!primID.empty());
        return &primID[0];
    };

    void calcAndPrintOverlap() const;

    //  protected:
    /*! node vector */
    std::vector<Node> node;
    /*! item list. The builder allocates this array, and fills it with
      ints that refer to the primitives; it's up to the */
    std::vector<int64> primID;
    /*! node reference to the root node */
    uint64 rootRef;
    const box4f &getBounds() const
    {
        return overallBounds;
    }
protected:
    box4f overallBounds;
};
}
