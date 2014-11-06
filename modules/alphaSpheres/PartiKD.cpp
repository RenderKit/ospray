/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// this module
#include "PartiKD.h"

namespace ospray {
  using std::cout; 
  using std::endl;

  struct Bigger {
    PartiKD3InPlace *tree;
    size_t dim;
    Bigger(size_t dim, PartiKD3InPlace *tree) : tree(tree), dim(dim) {};
    inline bool operator() (size_t a, size_t b) const { return tree->particle[a].position[dim] > tree->particle[b].position[dim]; }
  };

  struct Smaller {
    PartiKD3InPlace *tree;
    size_t dim;
    Smaller(size_t dim, PartiKD3InPlace *tree) : tree(tree), dim(dim) {};
    inline bool operator() (size_t a, size_t b) const { return tree->particle[a].position[dim] <tree->particle[b].position[dim]; }
  };

  template<typename Ordering>
  void updateHeapRoot(PartiKD3InPlace *tree, const PartiKD3InPlace::SubTreeIterator &root, const Ordering &ordered)
  {
    PartiKD3InPlace::SubTreeIterator it = root;
    // PING;
    // PRINT(root);
    if (it.isValid()) 
    while (1) {
      // PRINT(it);
      if (it.isLeaf()) break;
      if (it.isValid(it.rightChildID())) {
        if (ordered(it.leftChildID(),it.rightChildID())) {
          // try swapping with left
          if (ordered(it,it.leftChildID()))
            break;
          std::swap(tree->particle[it],tree->particle[it.leftChildID()]);          
          it.gotoLeftChild();
        } else {
          if (ordered(it,it.rightChildID()))
            break;
          // try swapping with right
          std::swap(tree->particle[it],tree->particle[it.rightChildID()]);          
          it.gotoRightChild();
        }
      } else if (it.isValid(it.leftChildID())) {
        if (!ordered(it,it.leftChildID()))
          std::swap(tree->particle[it],tree->particle[it.leftChildID()]);
        break;
      }
    }
    // PING;
  }


  template<typename Ordering>
  void buildHeap(PartiKD3InPlace *tree, const PartiKD3InPlace::SubTreeIterator &root, const Ordering &ordered)
  {
    PartiKD3InPlace::SubTreeIterator it = root;
    for (;it.isValid();it.gotoNext()) {
      size_t here = it;
      while (here != root) {
        size_t parent = PartiKD3InPlace::SubTreeIterator::parentID(here);
        if (ordered(parent,here)) break;
        std::swap(tree->particle[parent],tree->particle[here]);
        here = parent;
      }
    };
  }


  void buildRec(PartiKD3InPlace *tree, size_t rootID, const box3f &bounds)
  {
    if (rootID < 128) {
      cout << "bilding kd tree " << rootID << endl;
      PRINT(bounds);
    }
    PartiKD3InPlace::SubTreeIterator it(rootID,tree);
    if (!it.isValid()) return;
    if (it.isLeaf()) return;

    {
      PartiKD3InPlace::SubTreeIterator test(rootID,tree);
      while (test.isValid()) {
        if (!embree::conjoint(bounds,tree->particle[test].position)) {
          PING;
          PRINT(rootID);
          PRINT(test);
          PRINT(bounds);
          PRINT(tree->particle[test].position);
        }
        test.gotoNext();
      };
    }


    size_t splitDim = maxDim(bounds.size());
    tree->innerNodeInfo[it].setDim(splitDim);

    Bigger bigger(splitDim,tree);
    Smaller smaller(splitDim,tree);
    
    if (it.isValid(it.rightChildID())) {
      // has two full child trees.
      // PING;
      PartiKD3InPlace::SubTreeIterator lSubTree(it.leftChildID(),tree);
      PartiKD3InPlace::SubTreeIterator rSubTree(it.rightChildID(),tree);

      buildHeap(tree,lSubTree,bigger);
      buildHeap(tree,rSubTree,smaller);
      // PING;
      while (bigger(lSubTree,rSubTree)) {
        std::swap(tree->particle[lSubTree],tree->particle[rSubTree]);
        updateHeapRoot(tree,lSubTree,bigger);
        updateHeapRoot(tree,rSubTree,smaller);
      }
      if (bigger(it.leftChildID(),it)) {
        std::swap(tree->particle[it.leftChildID()],tree->particle[it]);
      }
      if (smaller(it.rightChildID(),it)) {
        std::swap(tree->particle[it.rightChildID()],tree->particle[it]);
      }
      box3f lBounds = bounds;
      box3f rBounds = bounds;
      lBounds.upper[splitDim] = rBounds.lower[splitDim] = tree->particle[it].position[splitDim];
      buildRec(tree,lSubTree,lBounds);
      buildRec(tree,rSubTree,rBounds);
    } else {
      // PING;
      // has only a single child on the left
      if (bigger(it.leftChildID(),it)) {
        std::swap(tree->particle[it.leftChildID()],tree->particle[it]);
      }
    }
    
  }

  void PartiKD3InPlace::buildSpatialComponent()
  {
    box3f bounds = embree::empty;
    for (int i=0;i<numParticles;i++)
      bounds.extend(particle[i].position);

    numInnerNodes = (numParticles)/2;
    innerNodeInfo = new InnerNodeInfo[numInnerNodes];
    buildRec(this,0,bounds);
  }
  
} // ::ospray
