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

#include "BlockBrickedVolume.ih"

//! The number of bits used to represent the width of a Block in voxels.
#define BLOCK_VOXEL_WIDTH_BITCOUNT (6)

//! The number of bits used to represent the width of a brick in voxels.
#define BRICK_VOXEL_WIDTH_BITCOUNT (2)

//! The number of bits used to represent the width of a block in bricks.
#define BLOCK_BRICK_WIDTH_BITCOUNT (BLOCK_VOXEL_WIDTH_BITCOUNT - BRICK_VOXEL_WIDTH_BITCOUNT)

//! The width of a block in voxels.
#define BLOCK_VOXEL_WIDTH (1 << BLOCK_VOXEL_WIDTH_BITCOUNT)

//! The width of a brick in voxels.
#define BRICK_VOXEL_WIDTH (1 << BRICK_VOXEL_WIDTH_BITCOUNT)

//! The width of a block in bricks.
#define BLOCK_BRICK_WIDTH (1 << BLOCK_BRICK_WIDTH_BITCOUNT)

//! The bits denoting the offset of a brick within a block.
#define BLOCK_BRICK_BITMASK (BLOCK_BRICK_WIDTH - 1)

//! The bits denoting the offset of a voxel within a brick.
#define BRICK_VOXEL_BITMASK (BRICK_VOXEL_WIDTH - 1)

//! The number of voxels contained in a block.
#define BLOCK_VOXEL_COUNT (BLOCK_VOXEL_WIDTH * BLOCK_VOXEL_WIDTH * BLOCK_VOXEL_WIDTH)

struct Address {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset of the voxel in the enclosing block.
  varying uint32 voxel;
};

inline void BlockBrickedVolume_getVoxelAddress(BlockBrickedVolume *uniform volume,
                                               const varying vec3i &index,
                                               varying Address &address)
{
  // Compute the 3D index of the block containing the brick containing the voxel.
  const vec3i blockIndex = index >> BLOCK_VOXEL_WIDTH_BITCOUNT;

  // Compute the 1D address of the block in the volume.
  address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  // Compute the 3D offset of the brick within the block containing the voxel.
  const vec3i brickOffset = bitwise_AND(index >> BRICK_VOXEL_WIDTH_BITCOUNT, BLOCK_BRICK_BITMASK);

  // Compute the 1D address of the brick in the block.
  const uint32 brickAddress
    = brickOffset.x
    + (brickOffset.y << BLOCK_BRICK_WIDTH_BITCOUNT)
    + (brickOffset.z << 2 * BLOCK_BRICK_WIDTH_BITCOUNT);

  // Compute the 3D offset of the voxel in the brick.
  const vec3i voxelOffset = bitwise_AND(index, BRICK_VOXEL_BITMASK);

  // Compute the 1D address of the voxel in the block.
  address.voxel
    = brickAddress  << (3 * BRICK_VOXEL_WIDTH_BITCOUNT)
    | voxelOffset.z << (2 * BRICK_VOXEL_WIDTH_BITCOUNT)
    | voxelOffset.y << BRICK_VOXEL_WIDTH_BITCOUNT
    | voxelOffset.x;
}



#define template_getVoxel(type)                                               \
inline void BlockBrickedVolume_getVoxel_##type(void *uniform _self,           \
                                               const varying vec3i &index,    \
                                               varying float &value)          \
{                                                                             \
  /* Cast to the actual volume subtype. */                                    \
  BlockBrickedVolume *uniform self = (BlockBrickedVolume *uniform)_self;      \
                                                                              \
  /* Compute the 1D address of the block in the volume                        \
   and the voxel in the block. */                                             \
  Address address;                                                            \
  BlockBrickedVolume_getVoxelAddress(self, index, address);                   \
                                                                              \
  /* The voxel value at the 1D address. */                                    \
  foreach_unique(blockID in address.block) {                                  \
    type *uniform blockPtr = (type *uniform)self->blockMem +                  \
        (BLOCK_VOXEL_COUNT * (uint64)blockID);                                \
    value = blockPtr[address.voxel];                                          \
  }                                                                           \
}

template_getVoxel(uint8);
template_getVoxel(int16);
template_getVoxel(uint16);
template_getVoxel(float);
template_getVoxel(double);
#undef template_getVoxel


inline void BlockBrickedVolume_allocateMemory(BlockBrickedVolume *uniform volume)
{
  // Memory may already have been allocated.
  if (volume->blockMem != NULL) return;

  // Volume size in blocks per dimension with padding to the nearest block.
  volume->blockCount = (volume->super.dimensions + BLOCK_VOXEL_WIDTH - 1) / BLOCK_VOXEL_WIDTH;

  // Volume size in blocks with padding.
  const uniform size_t blockCount = volume->blockCount.x * volume->blockCount.y * volume->blockCount.z;

  // allocate the large array of blocks
  uniform uint64 blockSize = BLOCK_VOXEL_COUNT * volume->voxelSize;
  volume->blockMem = malloc64(blockSize * (uint64)blockCount);

  if (volume->blockMem == NULL) {
    print("failed to allocate block memory!");
    return;
  }
}

/*! copy given block of voxels into the volume, where source[0] will
  be written to volume[targetCoord000] */
#define template_setRegion(type)                                              \
void BlockBrickedVolume_setRegion_##type(BlockBrickedVolume *uniform self,    \
                                         const void *uniform _source,         \
                                         const uniform vec3i &targetCoord000, \
                                         const uniform vec3i &regionSize,     \
                                         const uniform int taskIndex)         \
{                                                                             \
  const uniform uint32 region_y = taskIndex % regionSize.y;                   \
  const uniform uint32 region_z = taskIndex / regionSize.y;                   \
  const uniform uint64 runOfs = (uint64)regionSize.x *                        \
      (region_y + (uint64)regionSize.y * region_z);                           \
  const type *uniform run = (const type *uniform)_source + runOfs;            \
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);             \
  foreach (x = 0 ... regionSize.x) {                                          \
    Address address;                                                          \
    coord.x = targetCoord000.x + x;                                           \
    if (coord.x < 0 ||                                                        \
        coord.y < 0 ||                                                        \
        coord.z < 0 ||                                                        \
        coord.x >= self->super.dimensions.x ||                                \
        coord.y >= self->super.dimensions.y ||                                \
        coord.z >= self->super.dimensions.z                                   \
        )                                                                     \
      continue;                                                               \
                                                                              \
    BlockBrickedVolume_getVoxelAddress(self, coord, address);                 \
    foreach_unique(blockID in address.block) {                                \
      type *uniform blockPtr = ((type *uniform)self->blockMem)                \
          + blockID * (uint64)BLOCK_VOXEL_COUNT;                              \
      blockPtr[address.voxel] = run[x];                                       \
    }                                                                         \
  }                                                                           \
}

template_setRegion(uint8);
template_setRegion(int16);
template_setRegion(uint16);
template_setRegion(float);
template_setRegion(double);
#undef template_setRegion


void BlockBrickedVolume_Constructor(BlockBrickedVolume *uniform volume,
                                    /*! pointer to the c++-equivalent class of this entity */
                                    void *uniform cppEquivalent,
                                    const uniform int voxelType,
                                    const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&volume->super, cppEquivalent, dimensions);

  volume->blockMem = NULL;
  volume->voxelType = (OSPDataType) voxelType;

  if (volume->voxelType == OSP_UCHAR) {
    volume->voxelSize = sizeof(uniform uint8);
    volume->super.getVoxel = BlockBrickedVolume_getVoxel_uint8;
    volume->setRegion = &BlockBrickedVolume_setRegion_uint8;
  }
  else if (volume->voxelType == OSP_SHORT) {
    volume->voxelSize      = sizeof(uniform int16);
    volume->super.getVoxel = BlockBrickedVolume_getVoxel_int16;
    volume->setRegion      = &BlockBrickedVolume_setRegion_int16;
  }
  else if (volume->voxelType == OSP_USHORT) {
    volume->voxelSize      = sizeof(uniform uint16);
    volume->super.getVoxel = BlockBrickedVolume_getVoxel_uint16;
    volume->setRegion      = &BlockBrickedVolume_setRegion_uint16;
  }
  else if (volume->voxelType == OSP_FLOAT) {
    volume->voxelSize = sizeof(uniform float);
    volume->super.getVoxel = BlockBrickedVolume_getVoxel_float;
    volume->setRegion = &BlockBrickedVolume_setRegion_float;
  }
  else if (volume->voxelType == OSP_DOUBLE) {
    volume->voxelSize = sizeof(uniform double);
    volume->super.getVoxel = BlockBrickedVolume_getVoxel_double;
    volume->setRegion = &BlockBrickedVolume_setRegion_double;
  }
  else {
    print("#osp:block_bricked_volume: unknown voxel type\n");
    return;
  }

  // Allocate memory.
  BlockBrickedVolume_allocateMemory(volume);
}


export void *uniform BlockBrickedVolume_createInstance(void *uniform cppEquivalent,
                                                       const uniform int voxelType,
                                                       const uniform vec3i &dimensions)
{
  // The volume container.
  BlockBrickedVolume *uniform volume = uniform new uniform BlockBrickedVolume;
  BlockBrickedVolume_Constructor(volume, cppEquivalent, voxelType, dimensions);

  return volume;
}

export void BlockBrickedVolume_setRegion(void *uniform _self,
    // points to the first voxel to be copied. The voxels at 'source' MUST have
    // dimensions 'regionSize', must be organized in 3D-array order, and must
    // have the same voxel type as the volume.
    const void *uniform _source,
    // coordinates of the lower, left, front corner of the target region
    const uniform vec3i &regionCoords,
    // size of the region that we're writing to, MUST be the same as the
    // dimensions of source[][][]
    const uniform vec3i &regionSize,
    const uniform int taskIndex)
{
  // Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform self = (BlockBrickedVolume *uniform)_self;
  self->setRegion(self, _source, regionCoords, regionSize, taskIndex);
}

export void BlockBrickedVolume_freeVolume(void *uniform _self)
{
  BlockBrickedVolume *uniform self = (BlockBrickedVolume *uniform)_self;
  if (self->blockMem) {
    free64(self->blockMem);
  }
}
