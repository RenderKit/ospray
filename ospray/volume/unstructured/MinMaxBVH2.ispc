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
