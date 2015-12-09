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
    uint8 *uniform blockPtr = blockMem + (BLOCK_VOXEL_COUNT * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
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
    float *uniform blockPtr = blockMem + (BLOCK_VOXEL_COUNT * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
}

inline void BlockBrickedVolumeDouble_getVoxel(void *uniform _volume,
                                              const varying vec3i &index,
                                              varying float &value)
{
  // Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  double *uniform blockMem = (double *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;
  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    double *uniform blockPtr = blockMem + (BLOCK_VOXEL_COUNT * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
}

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

task void BBVUChar_setRegionTask(BlockBrickedVolume *uniform self,
                                 const uint8 *uniform source,
                                 const uniform vec3i &targetCoord000,
                                 const uniform vec3i &regionSize)
{
  const uniform uint32 region_y = taskIndex % regionSize.y;
  const uniform uint32 region_z = taskIndex / regionSize.y;
  const uniform uint64 runOfs = (uint64)regionSize.x * (region_y + (uint64)regionSize.y * region_z);
  const uint8 *uniform run = source + runOfs;
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);
  foreach (x = 0 ... regionSize.x) {
    Address address;
    coord.x = targetCoord000.x + x;
    if (coord.x < 0 ||
        coord.y < 0 ||
        coord.z < 0 ||
        coord.x >= self->super.dimensions.x ||
        coord.y >= self->super.dimensions.y ||
        coord.z >= self->super.dimensions.z
        )
      continue;
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
  const uniform uint64 runOfs = (uint64)regionSize.x * (region_y + (uint64)regionSize.y * region_z);
  const float *uniform run = source + runOfs;
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);
  foreach (x = 0 ... regionSize.x) {
    Address address;  
    coord.x = targetCoord000.x + x;
    if (coord.x < 0 ||
        coord.y < 0 ||
        coord.z < 0 ||
        coord.x >= self->super.dimensions.x ||
        coord.y >= self->super.dimensions.y ||
        coord.z >= self->super.dimensions.z
        )
      continue;

    BlockBrickedVolume_getVoxelAddress(self, coord, address);
    foreach_unique(blockID in address.block) {
      float *uniform blockPtr
        = ((float*uniform)self->blockMem) 
        + blockID * (uint64)BLOCK_VOXEL_COUNT;
      blockPtr[address.voxel] = run[x];
    }
  }
}

task void BBVDouble_setRegionTask(BlockBrickedVolume *uniform self,
                                  const double *uniform source, 
                                  const uniform vec3i &targetCoord000,
                                  const uniform vec3i &regionSize)
{
  const uniform uint32 region_y = taskIndex % regionSize.y;
  const uniform uint32 region_z = taskIndex / regionSize.y;
  const uniform uint64 runOfs = (uint64)regionSize.x * (region_y + (uint64)regionSize.y * region_z);
  const double *uniform run = source + runOfs;
  vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);
  foreach (x = 0 ... regionSize.x) {
    Address address;
    coord.x = targetCoord000.x + x;
    if (coord.x < 0 ||
        coord.y < 0 ||
        coord.z < 0 ||
        coord.x >= self->super.dimensions.x ||
        coord.y >= self->super.dimensions.y ||
        coord.z >= self->super.dimensions.z
        )
      continue;
    BlockBrickedVolume_getVoxelAddress(self, coord, address);
    foreach_unique(blockID in address.block) {
      double *uniform blockPtr
        = ((double*uniform)self->blockMem)
        + blockID * (uint64)BLOCK_VOXEL_COUNT;
      blockPtr[address.voxel] = run[x];
    }
  }
}
                                
/*! copy given block of voxels into the volume, where source[0] will
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

/*! copy given block of voxels into the volume, where source[0] will
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

/*! copy given block of voxels into the volume, where source[0] will
  be written to volume[targetCoord000] */
void BlockBrickedVolumeDouble_setRegion(void *uniform _volume, 
                                        const void *uniform _source, 
                                        const uniform vec3i &targetCoord000,
                                        const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] BBVDouble_setRegionTask((BlockBrickedVolume*uniform)_volume,
                                          (const double*uniform)_source,
                                          targetCoord000,
                                          regionSize);
}

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
    volume->super.getVoxel = BlockBrickedVolumeUChar_getVoxel;
    volume->setRegion = &BlockBrickedVolumeUChar_setRegion;
  }
  else if (volume->voxelType == OSP_FLOAT) {
    volume->voxelSize = sizeof(uniform float);
    volume->super.getVoxel = BlockBrickedVolumeFloat_getVoxel;
    volume->setRegion = &BlockBrickedVolumeFloat_setRegion;
  }
  else if (volume->voxelType == OSP_DOUBLE) {
    volume->voxelSize = sizeof(uniform double);    
    volume->super.getVoxel = BlockBrickedVolumeDouble_getVoxel;
    volume->setRegion = &BlockBrickedVolumeDouble_setRegion;
  }
  else {
    print("#osp:block_bricked_volume: unknown voxel type\n");
    return;
  }

  // Allocate memory.
  BlockBrickedVolume_allocateMemory(volume);
}

inline varying float BlockBrickedVolumeFloat_computeSample(void *uniform _volume, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  StructuredVolume *uniform volume = (StructuredVolume *uniform) _volume;

  // Transform the sample location into the local coordinate system.
  vec3f localCoordinates;
  volume->transformWorldToLocal(volume, worldCoordinates, localCoordinates);

  // Coordinates outside the volume are clamped to the volume bounds.
  const vec3f clampedLocalCoordinates = clamp(localCoordinates, make_vec3f(0.0f), 
                                              volume->localCoordinatesUpperBound);

  // Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);  
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = clampedLocalCoordinates - float_cast(voxelIndex_0);

  // Look up the voxel values to be interpolated.
  float voxelValue_000; 
  float voxelValue_001; 
  float voxelValue_010; 
  float voxelValue_011; 
  float voxelValue_100; 
  float voxelValue_101; 
  float voxelValue_110; 
  float voxelValue_111; 
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_000);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_001);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_010);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_011);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_100);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_101);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_110);
  BlockBrickedVolumeFloat_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_111);

  // Interpolate the voxel values.
  const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

  return volumeSample;
}


export void *uniform BlockBrickedVolume_createInstance(void *uniform cppEquivalent,
                                                       const uniform int voxelType, 
                                                       const uniform vec3i &dimensions)
{
  // The volume container.
  BlockBrickedVolume *uniform volume = uniform new uniform BlockBrickedVolume;

  BlockBrickedVolume_Constructor(volume, cppEquivalent, voxelType, dimensions);
  // if (voxelType == OSP_FLOAT) {
  //   print("using hand-inlined computesample fct for block-bricked\n");
  //   volume->super.super.computeSample = BlockBrickedVolumeFloat_computeSample;
  // }
  return volume;
}

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
