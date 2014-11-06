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
  
  struct PartiKD3InPlace {

    struct SubTreeIterator {
      size_t subTreeRootID;
      size_t numNodes;
      size_t numInnerNodes;
      size_t currentNodeID;
      size_t numNodesinLevel;
      size_t currentNodeIDinLevel;
      
      inline bool isAtRoot() { return currentNodeID == subTreeRootID; }
      inline bool isInner(size_t nodeID) { return nodeID < numInnerNodes; }
      inline bool isValid(size_t nodeID) { return nodeID < numNodes; }
      inline bool isInner() { return currentNodeID < numInnerNodes; }
      inline bool isValid() { return currentNodeID < numNodes; }
      inline bool isLeaf() { return currentNodeID >= numInnerNodes; }
      inline size_t parentID() { return (currentNodeID-1) / 2; }
      inline static size_t parentID(size_t ID) { return (ID-1) / 2; }
      inline size_t leftChildID() { return currentNodeID * 2 + 1; }
      inline size_t rightChildID() { return currentNodeID * 2 + 2; }
      inline operator size_t () const { return currentNodeID; }

      SubTreeIterator(size_t subTreeRootID, PartiKD3InPlace *tree) 
        : subTreeRootID(subTreeRootID),
          numNodes(tree->numParticles), numInnerNodes(tree->numInnerNodes),
          currentNodeID(subTreeRootID),
          numNodesinLevel(1),
          currentNodeIDinLevel(0)
      {}
      void gotoLeftChild() {
        currentNodeID = currentNodeID * 2 + 1;
        numNodesinLevel *= 2;
        currentNodeIDinLevel = 2 * currentNodeIDinLevel;
      }
      void gotoRightChild() {
        currentNodeID = currentNodeID * 2 + 2;
        numNodesinLevel *= 2;
        currentNodeIDinLevel = 2 * currentNodeIDinLevel+1;
      }
      void gotoNext() {
        currentNodeID ++;
        currentNodeIDinLevel++;
        if (currentNodeIDinLevel == numNodesinLevel) {
          currentNodeID = (currentNodeID - currentNodeIDinLevel) * 2 + 1;
          currentNodeIDinLevel = 0;
          numNodesinLevel *= 2;
        };
      }
    };

    struct Particle {
      vec3f  position;
      float  attribute;
    };

    struct InnerNodeInfo {
      uint32 getBinBits() const { return bits & ((1<<30)-1); }
      uint32 getDim() const { return bits >> 30; }

      void setBinBits(uint32 binBits) { bits = binBits | (getDim() << 30); }
      void setDim(uint32 dim) { bits = getBinBits() | (dim << 30); }
    private:
      uint32 bits;
    };

    Particle *particle;
    size_t numParticles;
    InnerNodeInfo *innerNodeInfo;
    size_t numInnerNodes;

    void buildSpatialComponent();
    void rebuildRangeBits() {};

    void build() {
      buildSpatialComponent();
      rebuildRangeBits();
    }

  };

} // ::ospray
