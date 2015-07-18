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

inline void BlockBrickedVolumeFloat_getVoxel(void *uniform _volume, 
                                             const varying vec3i &index, 
                                             varying float &value)
{
  // Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  float *uniform blockMem = (float *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  
  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    float *uniform blockPtr = blockMem + (BLOCK_VOXEL_COUNT * blockID);
    value = blockPtr[address.voxel];
  }
  if (any(value > .5f)) print("-> %\n",value);

}

inline void BlockBrickedVolumeUChar_getVoxel(void *uniform _volume, 
                                             const varying vec3i &index, 
                                             varying float &value)
{
  // Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  uint8 *uniform blockMem = (uint8 *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  
  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    uint8 *uniform blockPtr = blockMem + (BLOCK_VOXEL_COUNT * blockID);
    value = blockPtr[address.voxel];
  }
}

// inline void 
// BlockBrickedVolumeFloat_setVoxelFromRegion(void *uniform _volume, 
//                                            const void *uniform source, 
//                                            /*! the local coordinates in the region we're blitting */
//                                            const uniform vec3i &coordInRegion, 
//                                            /*! the size of the region we're copying */
//                                            const uniform vec3i &regionSize, 
//                                            /*! the coordinates where region[0,0,0] will be copied into */
//                                            const varying vec3i &targetCoord000)
// {
//   // Cast to the actual Volume subtype.
//   BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

//   // Cast to the actual voxel type.
//   float *uniform voxelData = (float *uniform) volume->voxelData;
//   const float *uniform sourceData = (const float *uniform) source;

//   foreach_unique(offsetZ in offset.z) {

//     const float *uniform sourceDataShifted = sourceData + (int64)offsetZ * count.y * count.x;

//     // Compute the 1D address of the block in the volume and the voxel in the block.
//     Address address;
//     BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

//     // The source voxel value.
//     float value = sourceDataShifted[offset.y * count.x + offset.x];

//     // Store the voxel value at the 1D address.
//     foreach_unique(block in address.block)
//       voxelData[block][address.voxel] = value;
//   }
// }

// inline void BlockBrickedVolumeUChar_setVoxel(void *uniform _volume, 
//                                              const void *uniform source, 
//                                              const uniform vec3i &index, 
//                                              const uniform vec3i &count, 
//                                              const varying vec3i &offset)
// {
//   // Cast to the actual Volume subtype.
//   BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

//   // Cast to the actual voxel type.
//   uint8 *uniform voxelData = (uint8 *uniform) volume->voxelData;
//   const uint8 *uniform sourceData = (const uint8 *uniform) source;

//   foreach_unique(offsetZ in offset.z) {

//     const uint8 *uniform sourceDataShifted = sourceData + (int64)offsetZ * count.y * count.x;

//     // Compute the 1D address of the block in the volume and the voxel in the block.
//     Address address;
//     BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

//     // The source voxel value.
//     uint8 value = sourceDataShifted[offset.y * count.x + offset.x];

//     // Store the voxel value at the 1D address.
//     foreach_unique(block in address.block)
//       voxelData[block][address.voxel] = value;
//   }
// }

inline void BlockBrickedVolume_allocateMemory(BlockBrickedVolume *uniform volume)
{
  // Memory may already have been allocated.
  if (volume->blockMem != NULL) return;

  // Volume size in blocks per dimension with padding to the nearest block.
  volume->blockCount = (volume->inherited.dimensions + BLOCK_VOXEL_WIDTH - 1) / BLOCK_VOXEL_WIDTH;

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

task void BBVUChar_setRegionTask(BlockBrickedVolume *uniform self,
                                 const uint8 *uniform source, 
                                 const uniform vec3i &targetCoord000,
                                 const uniform vec3i &regionSize)
{
  const uniform uint32 region_y = taskIndex % regionSize.y;
  const uniform uint32 region_z = taskIndex / regionSize.y;
  const uniform uint32 runOfs = regionSize.x * (region_y + regionSize.y * region_z);
  const uint8 *uniform run = source + runOfs;
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);
  foreach (x = 0 ... regionSize.x) {
    Address address;  
    coord.x = targetCoord000.x + x;
    BlockBrickedVolume_getVoxelAddress(self, coord, address);
    foreach_unique(blockID in address.block) {
      uint8 *uniform blockPtr
        = ((uint8*uniform)self->blockMem) 
        + blockID * (uint64)BLOCK_VOXEL_COUNT;
      blockPtr[address.voxel] = run[x];
    }
  }
}

task void BBVFloat_setRegionTask(BlockBrickedVolume *uniform self,
                                 const float *uniform source, 
                                 const uniform vec3i &targetCoord000,
                                 const uniform vec3i &regionSize)
{
  const uniform uint32 region_y = taskIndex % regionSize.y;
  const uniform uint32 region_z = taskIndex / regionSize.y;
  const uniform uint32 runOfs = regionSize.x * (region_y + regionSize.y * region_z);
  const float *uniform run = source + runOfs;
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);
  foreach (x = 0 ... regionSize.x) {
    Address address;  
    coord.x = targetCoord000.x + x;
    BlockBrickedVolume_getVoxelAddress(self, coord, address);
    foreach_unique(blockID in address.block) {
      float *uniform blockPtr
        = ((float*uniform)self->blockMem) 
        + blockID * (uint64)BLOCK_VOXEL_COUNT;
      blockPtr[address.voxel] = run[x];
    }
  }
}
                                 
/*! copy given block of pixels into the volume, where source[0] will
  be written to volume[targetCoord000] */
void BlockBrickedVolumeUChar_setRegion(void *uniform _volume, 
                                       const void *uniform _source, 
                                       const uniform vec3i &targetCoord000,
                                       const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] BBVUChar_setRegionTask((BlockBrickedVolume*uniform)_volume,
                                         (const uint8*uniform)_source,
                                         targetCoord000,
                                         regionSize);
}

  /*! copy given block of pixels into the volume, where source[0] will
    be written to volume[targetCoord000] */
void BlockBrickedVolumeFloat_setRegion(void *uniform _volume, 
                                       const void *uniform _source, 
                                       const uniform vec3i &targetCoord000,
                                       const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] BBVFloat_setRegionTask((BlockBrickedVolume*uniform)_volume,
                                         (const float*uniform)_source,
                                         targetCoord000,
                                         regionSize);
}



void BlockBrickedVolume_Constructor(BlockBrickedVolume *uniform volume, 
                                    /*! pointer to the c++-equivalent class of this entity */
                                    void *uniform cppEquivalent,
                                    const uniform int voxelType, 
                                    const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&volume->inherited, cppEquivalent, dimensions);

  volume->blockMem = NULL;
  volume->voxelType = (OSPDataType) voxelType;
  volume->voxelSize
    = (volume->voxelType == OSP_FLOAT) 
    ? sizeof(uniform float) 
    : sizeof(uniform uint8);
  volume->inherited.getVoxel
    = (volume->voxelType == OSP_FLOAT) 
    ? BlockBrickedVolumeFloat_getVoxel 
    : BlockBrickedVolumeUChar_getVoxel;
  volume->setRegion
    = (volume->voxelType == OSP_FLOAT)
    ? &BlockBrickedVolumeFloat_setRegion
    : &BlockBrickedVolumeUChar_setRegion;

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

// task void BlockBrickedVolume_setRegionTask(BlockBrickedVolume *uniform self, 
//                                            const void *uniform source, 
//                                            const uniform vec3i &index, 
//                                            const uniform vec3i &count)
// {
//   // Offset from task index.
//   const uniform int offsetY = taskIndex % count.y;
//   const uniform int offsetZ = taskIndex / count.y;

//   foreach (x = 0 ... count.x) {
//       const vec3i offset = make_vec3i(x, offsetY, offsetZ);
//       self->setVoxel(self, source, index, count, offset);
//     }
// }

export void 
BlockBrickedVolume_setRegion(void *uniform _self,
                             /* points to the first voxel to be copies. The
                                voxels at 'soruce' MUST have dimensions
                                'regionSize', must be organized in 3D-array
                                order, and must have the same voxel type as the
                                volume.*/
                             const void *uniform _source,
                             /*! coordinates of the lower, left, front corner of
                               the target region.*/
                             const uniform vec3i &regionCoords,
                             /*! size of the region that we're writing to; MUST
                               be the same as the dimensions of source[][][] */
                             const uniform vec3i &regionSize)
{
  // Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform self = (BlockBrickedVolume *uniform)_self;
  self->setRegion(_self,_source,regionCoords,regionSize);
}
