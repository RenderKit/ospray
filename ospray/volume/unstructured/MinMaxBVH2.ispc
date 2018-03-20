// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "MinMaxBVH2.ih"

void traverse(uniform MinMaxBVH2 &bvh,
              void *uniform userPtr,
              uniform intersectAndSamplePrim sampleFunc,
              float &result,
              const vec3f &samplePos)
{
  uniform int64 nodeRef = bvh.rootRef;
  uniform unsigned int8 *uniform node0ptr =
      (uniform unsigned int8 *uniform)bvh.node;
  uniform unsigned int8 *uniform primID0ptr =
      (uniform unsigned int8 *uniform)bvh.primID;
  uniform int64 nodeStack[32];
  uniform int64 stackPtr = 0;

  uniform MinMaxBVH2Node *uniform root =
      (uniform MinMaxBVH2Node * uniform)(node0ptr + ((nodeRef & 0x7) & ~(7LL)));

  while (1) {
    uniform int64 numPrimsInNode = nodeRef & 0x7;
    if (numPrimsInNode == 0) {  // intermediate node
      uniform MinMaxBVH2Node *uniform nodePair =
          (uniform MinMaxBVH2Node * uniform)(node0ptr + (nodeRef & ~(7LL)));
      const bool in0 = pointInAABBTest(nodePair[0], samplePos);
      const bool in1 = pointInAABBTest(nodePair[1], samplePos);

      if (any(in0)) {
        if (any(in1)) {
          nodeStack[stackPtr++] = nodePair[1].childRef;
          nodeRef               = nodePair[0].childRef;
          continue;
        } else {
          nodeRef = nodePair[0].childRef;
          continue;
        }
      } else {
        if (any(in1)) {
          nodeRef = nodePair[1].childRef;
          continue;
        } else {
          // Do nothing, just pop.
        }
      }
    } else {  // leaf, test primitives
      uniform int64 *uniform primIDPtr =
          (uniform int64 * uniform)(primID0ptr + (nodeRef & ~(7LL)));
      for (uniform int i = 0; i < numPrimsInNode; i++) {
        uniform uint64 primRef = primIDPtr[i];

        // Traverse the bvh in the piece, and if we have a valid sample at the
        // position return
        if (sampleFunc(userPtr,
                       primRef,
                       result,
                       samplePos,
                       root->range_lo,
                       root->range_hi)) {
          return;
        }
      }
    }
    if (stackPtr == 0) {
      return;
    }
    --stackPtr;
    nodeRef = nodeStack[stackPtr];
  }
}

inline uniform bool inIsoRange(uniform vec2f isoRange,
                               const uniform MinMaxBVH2Node &rn)
{
  if ((isoRange.x <= rn.range_hi) && (rn.range_lo <= isoRange.y))
    return true;
  return false;
}

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

void traverseRay(uniform MinMaxBVH2 &bvh,
                 void *uniform userPtr,
                 uniform vec2f isoRange,
                 uniform intersectIsoRay isoRayFunc,
                 uniform float *uniform isovalues,
                 uniform int numIsovalues,
                 uniform size_t geomID,
                 varying Ray &ray)
{
  const vec3f rdir = rcp(ray.dir);
  const vec3f rorg = neg(ray.org * rdir);

  uniform int64 nodeRef = bvh.rootRef;
  uniform int64 stackPtr = 0;
  uniform int64 nodeStack[32];
  varying float distStack[32];
  uniform unsigned int8 *uniform node0ptr
    = (uniform unsigned int8 *uniform)bvh.node;
  uniform unsigned int8 *uniform primID0ptr
    = (uniform unsigned int8 *uniform)bvh.primID;

  while (1) {
    uniform int64 numPrimsInNode = nodeRef & 0x7;
    if (numPrimsInNode == 0) {
      // inner node
      uniform MinMaxBVH2Node *uniform nodePair
        = (uniform MinMaxBVH2Node *uniform)(node0ptr + (nodeRef & ~(7LL)));
      float dist0, dist1;
      bool hit0 = inIsoRange(isoRange, nodePair[0])
        && intersects(ray,rorg,rdir,nodePair[0],dist0);
      bool hit1 = inIsoRange(isoRange, nodePair[1])
        && intersects(ray, rorg, rdir, nodePair[1],dist1);
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
      uniform int64 *uniform primIDPtr
        = (uniform int64 *uniform)(primID0ptr + (nodeRef & ~(7LL)));
      for (uniform int i=0;i<numPrimsInNode;i++) {
        uniform uint64 primRef = primIDPtr[i];
        isoRayFunc(userPtr, isovalues, numIsovalues, geomID, primRef, ray);
        // we can't early exit here "if (ray.primID >= 0) return;" because
        // there is no guarantee we visit nodes in ray order; need to hit
        // each and find the closest
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
