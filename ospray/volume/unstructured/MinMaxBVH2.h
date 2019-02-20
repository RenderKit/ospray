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

#pragma once

// ospray
#include "common/Data.h"
#include "common/Model.h"

namespace ospray {

  /*! defines a (binary) BVH with some float min/max value per
      node. The BVH itself does not specify the primitive type it is
      used with, or what those min/max values represent. */
  struct MinMaxBVH2
  {
    /*! a node in a MinMaxBVH: a (4D-)bounding box, plus a child/leaf reference */
    struct Node : public box4f
    {
      uint64 childRef;
    };

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
               const size_t numPrims);

    const void *nodePtr() const;

    const int64 *itemListPtr() const;

    uint64 rootRef() const;

   private:
    void buildRec(const size_t nodeID,
                  const box4f *const primBounds,
                  int64 *tmp_primID,
                  const size_t begin,
                  const size_t end);

    const box4f &bounds() const;

    // Data members //

    /*! node vector */
    std::vector<Node> node;
    /*! item list. The builder allocates this array, and fills it with
      ints that refer to the primitives; it's up to the */
    std::vector<int64> primID;
    /*! node reference to the root node */
    uint64 root;

    box4f overallBounds;
  };
}
