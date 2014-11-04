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
    if (fabs(f) < 1e-15) return 1e-15; 
    return f;
  }
  __forceinline float my_safeArea(const box4f &b) 
  { 
    vec4f size = b.upper - b.lower;
    float f =  size.x*size.y+size.x*size.z+size.y*size.z;
    if (fabs(f) < 1e-15) return 1e-15; 
    return f;
  }

  __forceinline vec4f make_vec4f(vec3f v, float w)
  { return vec4f(v.x,v.y,v.z,w); }

  void MinMaxBVH::buildRec(const size_t nodeID,
                           PrimAbstraction *pa,
                           const size_t begin, 
                           const size_t end)
  {
    box3f bounds3     = embree::empty;
    box3f centBounds3 = embree::empty;
    float ctr[3];
    for (size_t i=begin;i<end;i++) {
      box3f pb = pa->boundsOf(primID[i]);
      bounds3.extend(pb);
      centBounds3.extend(embree::center(pb));
    }

    const box3f centBounds = centBounds3; // ((vec3f&)centBounds4.lower, (vec3f&)centBounds4.upper);
    node[nodeID].lower = make_vec4f(bounds3.lower,0);
    node[nodeID].upper = make_vec4f(bounds3.upper,0);
                                    
    size_t dim = maxDim(centBounds.size());
    (vec3f&)ctr = embree::center(centBounds);
    float  pos = ctr[dim];
    float costNoSplit = 1+(end-begin);

    size_t l = begin;
    size_t r = end-1;
    box3f lBounds = embree::empty;
    box3f rBounds = embree::empty;

    while (1) {
      while (l <= r && embree::center(pa->boundsOf(primID[l]))[dim] < pos) {
        lBounds.extend(pa->boundsOf(primID[l]));
        ++l;
      }
      // l now points to a element on the RIGHT side
      while (l < r && embree::center(pa->boundsOf(primID[r]))[dim] >= pos) {
        rBounds.extend(pa->boundsOf(primID[r]));
        --r;
      }
      // r is now either same as l, OR pointing to element that belong to LEFT side
      if (l >= r)
        break;
      // cout << "swapping " << l << "," << (r)<< endl;
      std::swap(primID[l],primID[r]);
      // pa->swapPrims(l,r);
      lBounds.extend(pa->boundsOf(primID[l]));
      rBounds.extend(pa->boundsOf(primID[r]));
      ++l; --r;
    };
    
    float costIfSplit 
      = 1 + (1.f/my_safeArea(bounds3))*(my_safeArea(lBounds)*(l-begin)+
                                        my_safeArea(rBounds)*(end-l));
    if (
#ifdef LEAF_THRESHOLD
        (end-begin) <= LEAF_THRESHOLD || 
#endif
        ((costIfSplit >= costNoSplit) && ((end-begin) <= 3))
        || (l==begin)
        || (l==end)
        ) {
      int numChildren = end-begin;
      if (numChildren >= 4) {
        PRINT(begin);
        PRINT(bounds3);
        PRINT(my_safeArea(bounds3));
        PRINT(my_safeArea(lBounds));
        PRINT(my_safeArea(rBounds));
        PRINT(end);
        PRINT(l);
        PRINT(r);
        PRINT(costIfSplit);
        PRINT(costNoSplit);
      }
      assert(numChildren < 4);
      node[nodeID].childRef = (numChildren) + begin*4; //(end-begin) + begin*sizeof(primID[0]);

    } else {
      assert(l > begin);
      assert(l < end);
      for (int i=begin;i<l;i++) {
        (vec3f&)ctr = embree::center(pa->boundsOf(primID[i]));
        // vec4f ctr = embree::center(primBounds[primID[i]]);
        assert(ctr[dim] < pos);
      }
      for (int i=l;i<end;i++) {
        (vec3f&)ctr = embree::center(pa->boundsOf(primID[i]));
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

  void MinMaxBVH::updateRanges(PrimAbstraction *pa)
  {
    for (int nodeID=node.size()-1; nodeID>=0; --nodeID) {
      int numInNode = node[nodeID].childRef & 0x3;

      if (numInNode == 0) {
        size_t childID = node[nodeID].childRef / sizeof(Node);
        node[nodeID].lower.w = std::min(node[childID+0].lower.w,node[childID+1].lower.w);
        node[nodeID].upper.w = std::max(node[childID+0].upper.w,node[childID+1].upper.w);
      } else {
        size_t begin = node[nodeID].childRef / 4;
        node[nodeID].lower.w = +std::numeric_limits<float>::infinity();
        node[nodeID].upper.w = -std::numeric_limits<float>::infinity();
        for (size_t i=0;i<numInNode;i++) {
          float attr = pa->attributeOf(this->primID[begin+i]);
          node[nodeID].lower.w = std::min(node[nodeID].lower.w,attr);
          node[nodeID].upper.w = std::max(node[nodeID].upper.w,attr);
        }
      }
    }
  }
  
  void MinMaxBVH::initialBuild(PrimAbstraction *pa)
  {
    if (!this->node.empty()) {
      std::cout << "*REBUILDING* MinMaxBVH2!?" << std::endl;
    }
    size_t numPrims = pa->numPrims();
    this->node.clear();
    this->node.push_back(Node());
    this->node.push_back(Node());

    primID.resize(numPrims);
    for (int i=0;i<numPrims;i++) primID[i] = i;
    buildRec(0,pa,0,numPrims);
    updateRanges(pa);
    this->node.resize(this->node.size());

    rootRef = node[0].childRef;
  }
  
}

