// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/volume/BlockBrickedVolume.ih"

//! The number of bits used to represent the width of a Block in voxels.
#define BLOCK_VOXEL_WIDTH_BITCOUNT (8)

//! The number of bits used to represent the width of a brick in voxels.
#define BRICK_VOXEL_WIDTH_BITCOUNT (4)

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

inline void BlockBrickedVolume_getVoxelAddress(BlockBrickedVolume *uniform volume, const varying vec3i &index, varying Address &address)
{
  //! Compute the 3D index of the block containing the brick containing the voxel.
  const vec3i blockIndex = index >> BLOCK_VOXEL_WIDTH_BITCOUNT;

  //! Compute the 1D address of the block in the volume.
  address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  //! Compute the 3D offset of the brick within the block containing the voxel.
  const vec3i brickOffset = bitwise_AND(index >> BRICK_VOXEL_WIDTH_BITCOUNT, BLOCK_BRICK_BITMASK);

  //! Compute the 1D address of the brick in the block.
  const uint32 brickAddress = brickOffset.x + (brickOffset.y << BLOCK_BRICK_WIDTH_BITCOUNT) + (brickOffset.z << 2 * BLOCK_BRICK_WIDTH_BITCOUNT);

  //! Compute the 3D offset of the voxel in the brick.
  const vec3i voxelOffset = bitwise_AND(index, BRICK_VOXEL_BITMASK);

  //! Compute the 1D address of the voxel in the block.
  address.voxel = brickAddress << 3 * BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.z << 2 * BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.y << BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.x;
}

inline void BlockBrickedVolumeFloat_getVoxel(void *uniform _volume, const varying vec3i &index, varying float &value)
{
  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  float **uniform voxelData = (float **uniform) volume->voxelData;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  //! The voxel value at the 1D address.
  foreach_unique(block in address.block)
    value = voxelData[block][address.voxel];
}

inline void BlockBrickedVolumeUChar_getVoxel(void *uniform _volume, const varying vec3i &index, varying float &value)
{
  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  uint8 **uniform voxelData = (uint8 **uniform) volume->voxelData;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  //! The voxel value at the 1D address.
  foreach_unique(block in address.block)
    value = voxelData[block][address.voxel];
}

inline void BlockBrickedVolumeFloat_setVoxel(void *uniform _volume, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset)
{
  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  float **uniform voxelData = (float **uniform) volume->voxelData;  const float *uniform sourceData = (const float *uniform) source;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

  //! The source voxel value.
  float value = sourceData[(offset.z * volume->inherited.dimensions.y + offset.y) * volume->inherited.dimensions.x + offset.x];

  //! Store the voxel value at the 1D address.
  foreach_unique(block in address.block) voxelData[block][address.voxel] = value;
}

inline void BlockBrickedVolumeUChar_setVoxel(void *uniform _volume, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset)
{
  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  uint8 **uniform voxelData = (uint8 **uniform) volume->voxelData;  const uint8 *uniform sourceData = (const uint8 *uniform) source;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

  //! The source voxel value.
  uint8 value = sourceData[(offset.z * volume->inherited.dimensions.y + offset.y) * volume->inherited.dimensions.x + offset.x];

  //! Store the voxel value at the 1D address.
  foreach_unique(block in address.block) voxelData[block][address.voxel] = value;
}

inline void BlockBrickedVolume_allocateMemory(BlockBrickedVolume *uniform volume)
{
  //! The ISPC compiler fails during allocation of pointer types.
  typedef void *uniform Block;

  //! Memory may already have been allocated.
  if (volume->voxelData != NULL) return;

  //! Volume size in blocks per dimension with padding to the nearest block.
  volume->blockCount = (volume->inherited.dimensions + BLOCK_VOXEL_WIDTH - 1) / BLOCK_VOXEL_WIDTH;

  //! Volume size in blocks with padding.
  const uniform size_t blockCount = volume->blockCount.x * volume->blockCount.y * volume->blockCount.z;

  //! Allocate storage for pointers to the voxel blocks.
  volume->voxelData = uniform new uniform Block[blockCount];  if (volume->voxelData == NULL) return;

  //! Allocate storage for the individual voxel blocks.
  for (uniform size_t i=0 ; i < blockCount ; i++) volume->voxelData[i] = (void *uniform)(uniform new uniform uint8[BLOCK_VOXEL_COUNT * volume->voxelSize]);
}

void BlockBrickedVolume_Constructor(BlockBrickedVolume *uniform volume, const uniform int voxelType, const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&volume->inherited, dimensions);

  volume->voxelType = (OSPDataType) voxelType;
  volume->voxelSize = (volume->voxelType == OSP_FLOAT) ? sizeof(uniform float) : sizeof(uniform uint8);
  volume->inherited.getVoxel = (volume->voxelType == OSP_FLOAT) ? BlockBrickedVolumeFloat_getVoxel : BlockBrickedVolumeUChar_getVoxel;
  volume->setVoxel = (volume->voxelType == OSP_FLOAT) ? BlockBrickedVolumeFloat_setVoxel : BlockBrickedVolumeUChar_setVoxel;

  //! Allocate memory.
  BlockBrickedVolume_allocateMemory(volume);
}

export void *uniform BlockBrickedVolume_createInstance(const uniform int voxelType, const uniform vec3i &dimensions)
{
  //! The volume container.
  BlockBrickedVolume *uniform volume = uniform new uniform BlockBrickedVolume;

  BlockBrickedVolume_Constructor(volume, voxelType, dimensions);

  return volume;
}

export void BlockBrickedVolume_setRegion(void *uniform _self, const void *uniform source, const uniform vec3i &index, const uniform vec3i &count)
{
  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform self = (BlockBrickedVolume *uniform)_self;

  //! Copy voxel data from memory into the volume.
  foreach (z = 0 ... count.z, y = 0 ... count.y, x = 0 ... count.x) {
    const vec3i offset = make_vec3i(x, y, z);
    self->setVoxel(self, source, index, offset);
  }
}
