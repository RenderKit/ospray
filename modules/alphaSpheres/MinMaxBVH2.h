/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray d
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"

namespace ospray {

  /*! defines a (binary) BVH with some float min/max value per
      node. The BVH itself does not specify the primitive type it is
      used with, or what those min/max values represent. */
  struct MinMaxBVH {
    struct PrimAbstraction {
      virtual size_t numPrims() = 0;
      // virtual size_t sizeOfPrim() = 0;
      // virtual void swapPrims(size_t aID, size_t bID) = 0;
      virtual box3f boundsOf(size_t primID) = 0;
      virtual float attributeOf(size_t primID) = 0;
    };

    /*! a node in a MinMaxBVH: a (4D-)bounding box, plus a child/leaf refence */
    struct Node : public box4f {
      uint64 childRef;
    };
    void buildRec(const size_t nodeID, 
                  PrimAbstraction *pa, const size_t begin, const size_t end);
    void initialBuild(PrimAbstraction *pa);
    void updateRanges(PrimAbstraction *pa);

    /*! to allow passing this pointer to ISCP: */
    const void *getNodePtr() const { assert(!node.empty()); return &node[0]; };
    /*! to allow passing this pointer to ISCP: */
    // const int64 *getItemListPtr() const { assert(!primID.empty()); return &primID[0]; };
    
    //  protected:
    /*! node vector */
    std::vector<Node> node;
    std::vector<uint32> primID;
    /*! node reference to the root node */
    uint64 rootRef;
    const box4f &getBounds() const { return node[0]; }
  };

}
