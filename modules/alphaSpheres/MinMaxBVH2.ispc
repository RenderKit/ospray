/********************************************************************* * \
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "MinMaxBVH2.ih"

#define DBG(a) /**/

#if 0
// for now, xf::getMinMax is *varying* (though it probably shouldn't be!?)
inline bool rangeIsInteresting(TransferFunction *uniform xf,
                               const uniform MinMaxBVH2 &mm,
                               const uniform MinMaxBVH2Node &node)
{
  // get range of alpha values for given range of attribute values:

  // get attribute range over the entire tree (for normalization)
  const uniform float all_range_lo = mm.node[0].range_lo;
  const uniform float all_range_hi = mm.node[0].range_hi;

  // get attribute range for current subtree
  float node_range_lo = node.range_lo;
  float node_range_hi = node.range_hi;

  // normalize both values
  node_range_lo = (node_range_lo-all_range_lo) * rcp(all_range_hi-all_range_lo);
  node_range_hi = (node_range_hi-all_range_lo) * rcp(all_range_hi-all_range_lo);

  // evaluate alpha range for normalized attribute range
  vec2f range = xf->getMinMaxAlphaInRange(xf,node_range_lo,node_range_hi);

  // cull if max range (range.y) is below .5f
  return range.y/*max range*/ >= .5f;
}
#else
// this is what it _should_ be 
inline uniform bool rangeIsInteresting(TransferFunction *uniform xf,
                                       const uniform MinMaxBVH2 &mm,
                                       const uniform MinMaxBVH2Node &node)
{
  // uniform vec2f range = xf->getMinMaxAlphaInRange(xf,rn.range_lo,rn.range_hi);
  // return range.x <= .5f && range.y >= .5f;

  // get range of alpha values for given range of attribute values:

  // get attribute range over the entire tree (for normalization)
  const uniform float all_range_lo = mm.node[0].range_lo;
  const uniform float all_range_hi = mm.node[0].range_hi;

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
#endif


inline bool intersects(const Ray &ray, 
                       const vec3f rorg,
                       const vec3f rdir, 
                       const uniform MinMaxBVH2Node rn,
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

void MinMaxBVH2_traverse(uniform MinMaxBVH2 *uniform mmBVH,
                         void *uniform geomPtr,
                         TransferFunction *uniform xf,
                         uniform MinMaxBVH_intersectPrim intersectPrim,
                         uniform bool isShadowRay,
                         varying Ray &ray)
// void traverse(uniform AMRIsoSurfaceGeometry *uniform isoGeom,
//               uniform bool isShadowRay,
//               varying Ray &ray)
{
#if 1
  for (uniform int i=0;i<3*3*3;i++) {
    intersectPrim(geomPtr,i,isShadowRay,ray);
  }
  return;

#endif

  const vec3f rdir = rcp(ray.dir);
  const vec3f rorg = neg(ray.org * rdir);

  uniform int64 nodeRef = mmBVH->node[0].childRef;
  uniform int64 stackPtr = 0;
  uniform int64 nodeStack[32];
  varying float distStack[32];
  uniform unsigned int8 *uniform node0ptr
    = (uniform unsigned int8 *uniform)mmBVH->node;
  
  while (1) {
    uniform int64 numPrimsInNode = nodeRef & 0x7;
    if (numPrimsInNode == 0) {
      // inner node
      // print("inner node %\n",(nodeRef & ~(7LL)));
      uniform MinMaxBVH2Node *uniform nodePair 
        = (uniform MinMaxBVH2Node *uniform)(node0ptr + (nodeRef & ~(7LL)));
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
      // print("LEAF node %\n",(nodeRef & ~(7LL)));
      uniform uint64 leafBegin = nodeRef / 8; // & ~(7LL);
      for (uniform int i=0;i<numPrimsInNode;i++) { 
        // uniform uint64 primRef = primIDPtr[i];
        // isoGeom->intersectPrim(isoGeom,primRef,isShadowRay,ray);
        // print("intersection %\n",i);
        intersectPrim(geomPtr,leafBegin+i,isShadowRay,ray);
        // intersectCell(geometry,primRef,//isoGeom->hexa_cell[primID],
        //               isShadowRay,ray);
        if (isShadowRay && ray.primID >= 0)
          return;
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




// inline void printNode(uniform int64 nodeRef,uniform int childID, uniform MinMaxBVH2Node node) {
//   print("node % %\n bounds (% % %):(% % %)\n range (%:%)\n",
//         nodeRef,childID,
//         node.bounds_lo.x,
//         node.bounds_lo.y,
//         node.bounds_lo.z,
//         node.bounds_hi.x,
//         node.bounds_hi.y,
//         node.bounds_hi.z,
//         node.range_lo,
//         node.range_hi);
// }


