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

#include "CellRef.ih"
#include "FindStack.ih"
#include "../amr/AMR.ih"


  /* packet-based variant of findCell kernel */
extern CellRef findCell(const AMR *uniform self,
                        const varying vec3f &_worldSpacePos,
                        const float minWidth)
{
  const vec3f worldSpacePos = max(make_vec3f(0.f),
                                  min(self->worldBounds.upper,_worldSpacePos));
  const varying float *const uniform  samplePos = &worldSpacePos.x;

  uniform FindStack stack[16];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0);

  while (stackPtr > stack) {
    --stackPtr;
    if (stackPtr->active) {
      const uniform uint32 nodeID = stackPtr->nodeID;
      const uniform KDTreeNode &node = self->node[nodeID];
      if (isLeaf(node)) {
        const AMRLeaf *uniform leaf = &self->leaf[getOfs(node)];
        for (uniform int i=0;any(true);i++) {
          const AMRBrick *uniform brick = leaf->brickList[i];
          if (brick->cellWidth >= minWidth) {
            const vec3f relBrickPos
              = (worldSpacePos - brick->bounds.lower) * brick->bounds_scale;
            // brick coords: integer cell coordinates inside brick
            // OPT: the same calculations as below, just in
            // floats. this works as long as all values we calculate
            // with are fraction-less values (so essentially ints) and
            // fit into 24 bits mantissa (which they easily should for
            // any brick
            const vec3f f_bc = floor(relBrickPos * brick->f_dims);
            CellRef ret;
            const uint32 idx = (int)(f_bc.x + brick->f_dims.x*(f_bc.y+brick->f_dims.y*(f_bc.z)));
            ret.pos = brick->bounds.lower + f_bc*brick->cellWidth;
            ret.value = brick->value[idx];
            ret.width = brick->cellWidth;
            return ret;
          }
        }
      } else {
        const uniform uint32 childID = getOfs(node);
        if (samplePos[getDim(node)] >= getPos(node)) {
          stackPtr = pushStack(stackPtr,childID+1);
        } else {
          stackPtr = pushStack(stackPtr,childID);
        }
      }
    }
  }
}

extern CellRef findLeafCell(const AMR *uniform self,
                            const varying vec3f &_worldSpacePos)
{
  const vec3f worldSpacePos = max(make_vec3f(0.f),
                                  min(self->worldBounds.upper,_worldSpacePos));
  const varying float *const uniform  samplePos = &worldSpacePos.x;

  uniform FindStack stack[16];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0);

  while (stackPtr > stack) {
    --stackPtr;
    if (stackPtr->active) {
      const uniform uint32 nodeID = stackPtr->nodeID;
      const uniform KDTreeNode node = self->node[nodeID];
      if (isLeaf(node)) {
        const AMRLeaf *uniform leaf = &self->leaf[getOfs(node)];
        const AMRBrick *uniform brick = leaf->brickList[0];
        const vec3f relBrickPos
          = (worldSpacePos - brick->bounds.lower) * brick->bounds_scale;
        // brick coords: integer cell coordinates inside brick
        // OPT: the same calculations as below, just in
        // floats. this works as long as all values we calculate
        // with are fraction-less values (so essentially ints) and
        // fit into 24 bits mantissa (which they easily should for
        // any brick
        const vec3f f_bc = floor(relBrickPos * brick->f_dims);
        CellRef ret;
        const uint32 idx = (int)(f_bc.x + brick->f_dims.x*(f_bc.y+brick->f_dims.y*(f_bc.z)));
        ret.pos = brick->bounds.lower + f_bc*brick->cellWidth;
        ret.value = self->getVoxel((void*)brick->value, idx);
        ret.width = brick->cellWidth;
        return ret;
      } else {
        const uniform uint32 childID = getOfs(node);
        if (samplePos[getDim(node)] >= getPos(node)) {
          stackPtr = pushStack(stackPtr,childID+1);
        } else {
          stackPtr = pushStack(stackPtr,childID);
        }
      }
    }
  }
}
