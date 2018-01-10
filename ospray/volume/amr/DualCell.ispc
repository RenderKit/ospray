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

#include "DualCell.ih"

struct FindEightStack
{
  varying bool act_lo[3];
  varying bool act_hi[3];
  uniform int32 nodeID;
};

void findDualCell(const AMR *uniform self,
                  DualCell &dual)
{
  const vec3f _P0 = clamp(dual.cellID.pos,
                          make_vec3f(0.f),
                          self->maxValidPos);
  const vec3f _P1 = clamp(dual.cellID.pos+dual.cellID.width,
                          make_vec3f(0.f),
                          self->maxValidPos);

  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  const varying float *const uniform lo = p0;
  const varying float *const uniform hi = p1;

#define STACK_SIZE 64
  uniform FindEightStack stack[STACK_SIZE];
  uniform FindEightStack *uniform stackPtr = &stack[0];

  uniform int32 leafList[programCount*8];
  uniform int32 numLeaves = 0;

  bool act_lo[3] = { true, true, true };
  bool act_hi[3] = { true, true, true };
  uniform int nodeID = 0;
  while (any(true)) {
    const uniform KDTreeNode &node = self->node[nodeID];
    const uniform uint32 childID = getOfs(node);
    if (isLeaf(node)) {
      assert(numLeaves < (programCount * 8));
      leafList[numLeaves++] = childID;
      // go on to popping ...
    } else {
      const uniform int dim = getDim(node);
      const uniform float pos = getPos(node);
      const bool in_active
        = (act_lo[0] | act_hi[0])
        & (act_lo[1] | act_hi[1])
        & (act_lo[2] | act_hi[2]);
      const bool go_left
        = ((act_lo[dim] & (lo[dim] < pos)) |
           (act_hi[dim] & (hi[dim] < pos)))
        & in_active;
      const bool go_right
        = ((act_lo[dim] & (lo[dim] >= pos)) |
           (act_hi[dim] & (hi[dim] >= pos)))
        & in_active;

      if (none(go_right)) {
        // all to the left: go left and iterate
        nodeID = childID+0;
        continue;
      }

      if (none(go_left)) {
        // all to the left: go left and iterate
        nodeID = childID+1;
        continue;
      }

      // push right
      stackPtr->nodeID = childID+1;
      for (uniform int i=0;i<3;i++) {
        unmasked {
          stackPtr->act_lo[i] = act_lo[i];
          stackPtr->act_hi[i] = act_hi[i];
        }
      }
      stackPtr->act_lo[dim] &= (lo[dim] >= pos);
      stackPtr->act_hi[dim] &= (hi[dim] >= pos);
      ++stackPtr;
      assert(stackPtr-stack < STACK_SIZE);

      // go left
      nodeID = childID+0;
      act_lo[dim] &= (lo[dim] < pos);
      act_hi[dim] &= (hi[dim] < pos);
      continue;
    }
    // pop:
    if (stackPtr == stack) break;
    --stackPtr;
    for (uniform int i=0;i<3;i++) {
      unmasked {
        act_lo[i] = stackPtr->act_lo[i];
        act_hi[i] = stackPtr->act_hi[i];
      }
    }
    nodeID = stackPtr->nodeID;
  }

  // -------------------------------------------------------
  // now, process leaves we found
  // -------------------------------------------------------
  foreach_unique (desired_width in dual.cellID.width) {
    for (uniform int leafID=0;leafID<numLeaves;leafID++) {
      const AMRLeaf *uniform leaf = &self->leaf[leafList[leafID]];
      // assert(leaf);
      const bool valid_x0 = _P0.x >= leaf->bounds.lower.x & _P0.x < leaf->bounds.upper.x;
      const bool valid_y0 = _P0.y >= leaf->bounds.lower.y & _P0.y < leaf->bounds.upper.y;
      const bool valid_z0 = _P0.z >= leaf->bounds.lower.z & _P0.z < leaf->bounds.upper.z;

      const bool valid_x1 = _P1.x >= leaf->bounds.lower.x & _P1.x < leaf->bounds.upper.x;
      const bool valid_y1 = _P1.y >= leaf->bounds.lower.y & _P1.y < leaf->bounds.upper.y;
      const bool valid_z1 = _P1.z >= leaf->bounds.lower.z & _P1.z < leaf->bounds.upper.z;

      const bool anyValid =
        (valid_x0 | valid_x1) &
        (valid_y0 | valid_y1) &
        (valid_z0 | valid_z1);
      if (!(anyValid))
        continue;

      uniform int brickID = 0;
      uniform bool isLeaf = true;
      const AMRBrick *uniform brick = leaf->brickList[brickID];
      while (brick->cellWidth < desired_width) {
        brick = leaf->brickList[++brickID];
        isLeaf = false;
      }

      const float *uniform v = brick->value;
      const vec3f rp0 = (_P0 - brick->bounds.lower) * brick->bounds_scale;
      const vec3f rp1 = (_P1 - brick->bounds.lower) * brick->bounds_scale;

      const vec3f f_bc0 = floor(rp0 * brick->f_dims);
      const vec3f f_bc1 = floor(rp1 * brick->f_dims);

      // index offsets to neighbor cells
      const float f_idx_dx0 = f_bc0.x;
      const float f_idx_dy0 = f_bc0.y*brick->f_dims.x;
      const float f_idx_dz0 = f_bc0.z*brick->f_dims.x*brick->f_dims.y;

      const float f_idx_dx1 = f_bc1.x;
      const float f_idx_dy1 = f_bc1.y*brick->f_dims.x;
      const float f_idx_dz1 = f_bc1.z*brick->f_dims.x*brick->f_dims.y;

#define DOCORNER(X,Y,Z)                                                 \
      if (valid_z##Z & valid_y##Y & valid_x##X) {                       \
        const int idx = (int)(f_idx_dx##X+f_idx_dy##Y+f_idx_dz##Z);     \
        dual.value[Z*4+Y*2+X]       = v[idx];                           \
        dual.actualWidth[Z*4+Y*2+X] = brick->cellWidth;                 \
        dual.isLeaf[Z*4+Y*2+X]      = isLeaf;                           \
      }
      DOCORNER(0,0,0);
      DOCORNER(0,0,1);
      DOCORNER(0,1,0);
      DOCORNER(0,1,1);
      DOCORNER(1,0,0);
      DOCORNER(1,0,1);
      DOCORNER(1,1,0);
      DOCORNER(1,1,1);
#undef DOCORNER
    }
  }
}




void findMirroredDualCell(const AMR *uniform self,
                          const vec3i &mirror,
                          DualCell &dual)
{
  const vec3f _P0 = clamp(dual.cellID.pos,
                          make_vec3f(0.f),
                          self->maxValidPos);
  const vec3f _P1 = clamp(dual.cellID.pos+dual.cellID.width,
                          make_vec3f(0.f),
                          self->maxValidPos);

  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  const float lo[3] = { mirror.x?_P1.x:_P0.x, mirror.y?_P1.y:_P0.y, mirror.z?_P1.z:_P0.z };
  const float hi[3] = { mirror.x?_P0.x:_P1.x, mirror.y?_P0.y:_P1.y, mirror.z?_P0.z:_P1.z };

#define STACK_SIZE 64
  uniform FindEightStack stack[STACK_SIZE];
  uniform FindEightStack *uniform stackPtr = &stack[0];

  uniform int32 leafList[programCount*8];
  uniform int32 numLeaves = 0;

  bool act_lo[3] = { true, true, true };
  bool act_hi[3] = { true, true, true };
  uniform int nodeID = 0;
  while (any(true)) {
    const uniform KDTreeNode &node = self->node[nodeID];
    const uniform uint32 childID = getOfs(node);
    if (isLeaf(node)) {
      assert(numLeaves < (programCount * 8));
      leafList[numLeaves++] = childID;
      // go on to popping ...
    } else {
      const uniform int dim = getDim(node);
      const uniform float pos = getPos(node);
      const bool in_active
        = (act_lo[0] | act_hi[0])
        & (act_lo[1] | act_hi[1])
        & (act_lo[2] | act_hi[2]);
      const bool go_left
        = ((act_lo[dim] & (lo[dim] < pos)) |
           (act_hi[dim] & (hi[dim] < pos)))
        & in_active;
      const bool go_right
        = ((act_lo[dim] & (lo[dim] >= pos)) |
           (act_hi[dim] & (hi[dim] >= pos)))
        & in_active;

      if (none(go_right)) {
        // all to the left: go left and iterate
        nodeID = childID+0;
        continue;
      }

      if (none(go_left)) {
        // all to the left: go left and iterate
        nodeID = childID+1;
        continue;
      }

      // push right
      stackPtr->nodeID = childID+1;
      for (uniform int i=0;i<3;i++) {
        unmasked {
          stackPtr->act_lo[i] = act_lo[i];
          stackPtr->act_hi[i] = act_hi[i];
        }
      }
      stackPtr->act_lo[dim] &= (lo[dim] >= pos);
      stackPtr->act_hi[dim] &= (hi[dim] >= pos);
      ++stackPtr;

      // go left
      nodeID = childID+0;
      act_lo[dim] &= (lo[dim] < pos);
      act_hi[dim] &= (hi[dim] < pos);
      continue;
    }
    // pop:
    if (stackPtr == stack) break;
    --stackPtr;
    for (uniform int i=0;i<3;i++) {
      unmasked {
        act_lo[i] = stackPtr->act_lo[i];
        act_hi[i] = stackPtr->act_hi[i];
      }
    }
    nodeID = stackPtr->nodeID;
  }

  // -------------------------------------------------------
  // now, process leaves we found
  // -------------------------------------------------------
  foreach_unique (desired_width in dual.cellID.width) {
    for (uniform int leafID=0;leafID<numLeaves;leafID++) {
      const AMRLeaf *uniform leaf = &self->leaf[leafList[leafID]];
      // assert(leaf);
      const bool valid_x0 = lo[0] >= leaf->bounds.lower.x & lo[0] < leaf->bounds.upper.x;
      const bool valid_y0 = lo[1] >= leaf->bounds.lower.y & lo[1] < leaf->bounds.upper.y;
      const bool valid_z0 = lo[2] >= leaf->bounds.lower.z & lo[2] < leaf->bounds.upper.z;

      const bool valid_x1 = hi[0] >= leaf->bounds.lower.x & hi[0] < leaf->bounds.upper.x;
      const bool valid_y1 = hi[1] >= leaf->bounds.lower.y & hi[1] < leaf->bounds.upper.y;
      const bool valid_z1 = hi[2] >= leaf->bounds.lower.z & hi[2] < leaf->bounds.upper.z;

      const bool anyValid =
        (valid_x0 | valid_x1) &
        (valid_y0 | valid_y1) &
        (valid_z0 | valid_z1);
      if (!(anyValid))
        continue;

      uniform int brickID = 0;
      uniform bool isLeaf = true;
      const AMRBrick *uniform brick = leaf->brickList[brickID];
      while (brick->cellWidth < desired_width) {
        brick = leaf->brickList[++brickID];
        isLeaf = false;
      }

      const float *uniform v = brick->value;
      const vec3f rp0 = (make_vec3f(lo[0],lo[1],lo[2]) - brick->bounds.lower) * brick->bounds_scale;
      const vec3f rp1 = (make_vec3f(hi[0],hi[1],hi[2]) - brick->bounds.lower) * brick->bounds_scale;

      const vec3f f_bc0 = floor(rp0 * brick->f_dims);
      const vec3f f_bc1 = floor(rp1 * brick->f_dims);

      // index offsets to neighbor cells
      const float f_idx_dx0 = f_bc0.x;
      const float f_idx_dy0 = f_bc0.y*brick->f_dims.x;
      const float f_idx_dz0 = f_bc0.z*brick->f_dims.x*brick->f_dims.y;

      const float f_idx_dx1 = f_bc1.x;
      const float f_idx_dy1 = f_bc1.y*brick->f_dims.x;
      const float f_idx_dz1 = f_bc1.z*brick->f_dims.x*brick->f_dims.y;

#define DOCORNER(X,Y,Z)                                                 \
      if (valid_z##Z & valid_y##Y & valid_x##X) {                       \
        const int idx = (int)(f_idx_dx##X+f_idx_dy##Y+f_idx_dz##Z);     \
        dual.value[Z*4+Y*2+X]       = v[idx];                           \
        dual.actualWidth[Z*4+Y*2+X] = brick->cellWidth;                 \
        dual.isLeaf[Z*4+Y*2+X]      = isLeaf;                           \
      }
      DOCORNER(0,0,0);
      DOCORNER(0,0,1);
      DOCORNER(0,1,0);
      DOCORNER(0,1,1);
      DOCORNER(1,0,0);
      DOCORNER(1,0,1);
      DOCORNER(1,1,0);
      DOCORNER(1,1,1);
#undef DOCORNER
    }
  }
}
