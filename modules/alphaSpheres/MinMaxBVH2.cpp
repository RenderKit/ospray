/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

#include "MinMaxBVH2.h"

// num prims that _force_ a leaf; undef to revert to sah termination criterion
#define LEAF_THRESHOLD 2

namespace ospray {
  using std::cout;
  using std::endl;

  __forceinline float my_safeArea(const box3f &b) 
  { 
    vec3f size = b.upper - b.lower;
    float f = size.x*size.y+size.x*size.z+size.y*size.z;
    if (fabs(f) < 1e-8) return 1e-8; 
    return f;
  }
  __forceinline float my_safeArea(const box4f &b) 
  { 
    vec4f size = b.upper - b.lower;
    float f =  size.x*size.y+size.x*size.z+size.y*size.z;
    if (fabs(f) < 1e-8) return 1e-8; 
    return f;
  }

  void MinMaxBVH::buildRec(const size_t nodeID,
                           PrimAbstraction *pa,
                           const size_t begin, 
                           const size_t end)
  {
    box4f bounds4     = embree::empty;
    box4f centBounds4 = embree::empty;
    float ctr[4];
    for (size_t i=begin;i<end;i++) {
      box4f pb = pa->boundsOfPrim(i);
      bounds4.extend(pb);
      centBounds4.extend(embree::center(pb));
    }

    const box3f centBounds((vec3f&)centBounds4.lower, (vec3f&)centBounds4.upper);
    node[nodeID].lower = bounds4.lower;
    node[nodeID].upper = bounds4.upper;

    size_t dim = maxDim(centBounds.size());
    (vec3f&)ctr = embree::center(centBounds);
    float  pos = ctr[dim];
    float costNoSplit = 1+(end-begin);

    size_t l = begin;
    size_t r = end-1;
    box4f lBounds = embree::empty;
    box4f rBounds = embree::empty;

    while (1) {
      while (l <= r && embree::center(pa->boundsOfPrim(l))[dim] < pos) {
        lBounds.extend(pa->boundsOfPrim(l));
        ++l;
      }
      // l now points to a element on the RIGHT side
      while (l < r && embree::center(pa->boundsOfPrim(r))[dim] >= pos) {
        rBounds.extend(pa->boundsOfPrim(r));
        --r;
      }
      // r is now either same as l, OR pointing to element that belong to LEFT side
      if (l >= r)
        break;
      // cout << "swapping " << l << "," << (r)<< endl;
      pa->swapPrims(l,r);
      lBounds.extend(pa->boundsOfPrim(l));
      rBounds.extend(pa->boundsOfPrim(r));
      ++l; --r;
    };
    
    float costIfSplit 
      = 1 + (1.f/my_safeArea(bounds4))*(my_safeArea(lBounds)*(l-begin)+
                                        my_safeArea(rBounds)*(end-l));
    if (
#ifdef LEAF_THRESHOLD
        (end-begin) <= LEAF_THRESHOLD || 
#endif
        (costIfSplit >= costNoSplit) && ((end-begin) <= 7)
        || (l==begin)
        || (l==end)
        ) {
      int numChildren = end-begin;
      node[nodeID].childRef = (numChildren) + begin*8; //(end-begin) + begin*sizeof(primID[0]);
      assert(numChildren > 0);
      assert(numChildren < 8);
      // PING;
      // for (int i=begin;i<end;i++)
      //   PRINT(i);
       // PING;
    } else {
      // PING;
      assert(l > begin);
      assert(l < end);
      for (int i=begin;i<l;i++) {
        (vec4f&)ctr = embree::center(pa->boundsOfPrim(i));
        // vec4f ctr = embree::center(primBounds[primID[i]]);
        assert(ctr[dim] < pos);
      }
      for (int i=l;i<end;i++) {
        (vec4f&)ctr = embree::center(pa->boundsOfPrim(i));
        // vec4f ctr = embree::center(primBounds[primID[i]]);
        assert(ctr[dim] >= pos);
      }
      size_t childID = node.size();
      node[nodeID].childRef = childID * sizeof(Node);
      node.push_back(Node());
      node.push_back(Node());
      // cout << "rec: " << begin << " " << end << endl;
      buildRec(childID+0,pa,begin,l);
      buildRec(childID+1,pa,l,end);
    }
  }
  void MinMaxBVH::build(PrimAbstraction *pa)
  {
    if (!this->node.empty()) {
      std::cout << "*REBUILDING* MinMaxBVH2!?" << std::endl;
    }
    size_t numPrims = pa->numPrims();
    this->node.clear();
    this->node.push_back(Node());
    this->node.push_back(Node());


    // for (int i=0;i<pa->numPrims();i++) {
    //   PRINT(center(pa->boundsOfPrim(i)));
    // }
    // PING;
    buildRec(0,pa,0,numPrims);
    this->node.resize(this->node.size());

    // for (int i=0;i<pa->numPrims();i++) {
    //   PRINT(center(pa->boundsOfPrim(i)));
    // }
    rootRef = node[0].childRef;
  }
  
}

