/********************************************************************* * \
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "MinMaxBVH2.ih"

#define DBG(a) /**/

inline uniform bool rangeIsInteresting(TransferFunction *uniform xf,
                                       const uniform MinMaxBVH2 &mm,
                                       const uniform MinMaxBVH2Node &node)
{
  // get range of alpha values for given range of attribute values:

  // get attribute range over the entire tree (for normalization)
  const uniform float all_range_lo = mm.node[0].range_lo;
  const uniform float all_range_hi = mm.node[0].range_hi;

  if (all_range_hi == all_range_lo) return true;

  // get attribute range for current subtree
  uniform float node_range_lo = node.range_lo;
  uniform float node_range_hi = node.range_hi;

  // normalize both values
  node_range_lo = (node_range_lo-all_range_lo) * rcp(all_range_hi-all_range_lo);
  node_range_hi = (node_range_hi-all_range_lo) * rcp(all_range_hi-all_range_lo);

  // evaluate alpha range for normalized attribute range
  uniform vec2f range = xf->getMinMaxAlphaInRange(xf,node_range_lo,node_range_hi);

  // cull if max range (range.y) is below .5f
  return range.y/*max range*/ >= .5f;
}

inline bool intersects(const Ray &ray, 
                       const vec3f rorg,
                       const vec3f rdir, 
                       const uniform MinMaxBVH2Node &rn,
                       float &dist)
{
  const float t_lo_x = rn.bounds_lo.x * rdir.x + rorg.x;
  const float t_lo_y = rn.bounds_lo.y * rdir.y + rorg.y;
  const float t_lo_z = rn.bounds_lo.z * rdir.z + rorg.z;
  const float t_hi_x = rn.bounds_hi.x * rdir.x + rorg.x;
  const float t_hi_y = rn.bounds_hi.y * rdir.y + rorg.y;
  const float t_hi_z = rn.bounds_hi.z * rdir.z + rorg.z;
  const float t_nr_x = min(t_lo_x,t_hi_x);
  const float t_fr_x = max(t_lo_x,t_hi_x);
  const float t_nr_y = min(t_lo_y,t_hi_y);
  const float t_fr_y = max(t_lo_y,t_hi_y);
  const float t_nr_z = min(t_lo_z,t_hi_z);
  const float t_fr_z = max(t_lo_z,t_hi_z);
  const float t_nr = max4(ray.t0,t_nr_x,t_nr_y,t_nr_z);
  const float t_fr = min4(ray.t,t_fr_x,t_fr_y,t_fr_z);
  dist = t_nr;
  return t_nr <= t_fr;
}

void MinMaxBVH2_intersect(uniform MinMaxBVH2 *uniform mmBVH,
                         void *uniform geomPtr,
                         TransferFunction *uniform xf,
                         uniform MinMaxBVH_intersectPrim intersectPrim,
                         uniform int numPrims,
                         varying Ray &ray)
{
  const vec3f rdir = rcp(ray.dir);
  const vec3f rorg = neg(ray.org * rdir);

  uniform int64 nodeRef = mmBVH->node[0].childRef;
  uniform int64 stackPtr = 0;
  uniform int64 nodeStack[64];
  varying float distStack[64];
  uniform unsigned int8 *uniform node0ptr
    = (uniform unsigned int8 *uniform)mmBVH->node;
  const uniform uint32 *uniform primID = mmBVH->primID;
  
  while (1) {
    uniform int64 numPrimsInNode = nodeRef & 0x3;
    if (numPrimsInNode == 0) {
      uniform MinMaxBVH2Node *uniform nodePair 
        = (uniform MinMaxBVH2Node *uniform)(node0ptr + (nodeRef & ~(3LL)));
      float dist0, dist1;
      DBG(printNode(nodeRef,0,nodePair[0]);
          printNode(nodeRef,1,nodePair[1]));
      bool hit0 = rangeIsInteresting(xf,*mmBVH,nodePair[0])
        && intersects(ray,rorg,rdir,nodePair[0],dist0);
      bool hit1 = rangeIsInteresting(xf,*mmBVH,nodePair[1])
        && intersects(ray,rorg,rdir,nodePair[1],dist1);
      if (any(hit0)) {
        if (any(hit1)) {
          unmasked { distStack[stackPtr] = 1e20f; }
          if (any(hit0 & hit1 & dist0 < dist1)) {
            distStack[stackPtr]    = dist1;
            nodeStack[stackPtr++] = nodePair[1].childRef;
            nodeRef = nodePair[0].childRef;
            continue;
          } else {
            distStack[stackPtr]    = dist0;
            nodeStack[stackPtr++] = nodePair[0].childRef;
            nodeRef = nodePair[1].childRef;
            continue;
          }
        } else {
          nodeRef = nodePair[0].childRef;
          continue;
        }
      } else {
        if (any(hit1)) {
          nodeRef = nodePair[1].childRef;
          continue;
        } else {
          // do nothing, just pop.
        }
      }
    } else {
      // primitives: do intersection
      uniform uint64 leafBegin = nodeRef / 4;
      for (uniform int i=0;i<numPrimsInNode;i++) { 
        intersectPrim(geomPtr,primID[leafBegin+i],ray);
      }
    }
    while (1) {
      // now, go on popping from stack.
      if (stackPtr == 0) return;
      --stackPtr;
      if (none(distStack[stackPtr] < ray.t))
        continue;
      nodeRef = nodeStack[stackPtr];
      break;
    }
  }
}



void MinMaxBVH2_occluded(uniform MinMaxBVH2 *uniform mmBVH,
                         void *uniform geomPtr,
                         TransferFunction *uniform xf,
                         uniform MinMaxBVH_intersectPrim intersectPrim,
                         uniform int numPrims,
                         varying Ray &ray)
{
  const vec3f rdir = rcp(ray.dir);
  const vec3f rorg = neg(ray.org * rdir);

  uniform int64 nodeRef = mmBVH->node[0].childRef;
  uniform int64 stackPtr = 0;
  uniform int64 nodeStack[64];
  uniform unsigned int8 *uniform node0ptr
    = (uniform unsigned int8 *uniform)mmBVH->node;
  const uniform uint32 *uniform primID = mmBVH->primID;
  
  while (1) {
    uniform int64 numPrimsInNode = nodeRef & 0x3;
    if (numPrimsInNode == 0) {
      uniform MinMaxBVH2Node *uniform nodePair 
        = (uniform MinMaxBVH2Node *uniform)(node0ptr + (nodeRef & ~(3LL)));
      float dist0, dist1;
      DBG(printNode(nodeRef,0,nodePair[0]);
          printNode(nodeRef,1,nodePair[1]));
      bool hit0 = rangeIsInteresting(xf,*mmBVH,nodePair[0])
        && intersects(ray,rorg,rdir,nodePair[0],dist0);
      bool hit1 = rangeIsInteresting(xf,*mmBVH,nodePair[1])
        && intersects(ray,rorg,rdir,nodePair[1],dist1);
      if (any(hit0)) {
        if (any(hit1)) {
          nodeStack[stackPtr++] = nodePair[1].childRef;
          nodeRef = nodePair[0].childRef;
          continue;
        } else {
          nodeRef = nodePair[0].childRef;
          continue;
        }
      } else {
        if (any(hit1)) {
          nodeRef = nodePair[1].childRef;
          continue;
        } else {
          // do nothing, just pop.
        }
      }
    } else {
      // primitives: do intersection
      uniform uint64 leafBegin = nodeRef / 4;
      for (uniform int i=0;i<numPrimsInNode;i++) { 
        if (intersectPrim(geomPtr,primID[leafBegin+i],ray))
          return;
      }
    }
    while (1) {
      // now, go on popping from stack.
      if (stackPtr == 0) return;
      --stackPtr;
      nodeRef = nodeStack[stackPtr];
      break;
    }
  }
}


