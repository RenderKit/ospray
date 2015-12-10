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

#include "ospray/volume/PaddedBlockBrickedVolume.ih"

extern uniform bool g_dbg;

#define BLOCK_BITS (7)
#define BRICK_BITS (2)

#define BLOCK_WIDTH (1<<BLOCK_BITS)
#define BRICK_WIDTH (1<<BRICK_BITS)

#define BRICK_MASK (BRICK_WIDTH-1)

/*! size of one block, in voxels */
#define VOXELS_PER_BLOCK (BLOCK_WIDTH*BLOCK_WIDTH*BLOCK_WIDTH)


#define BRICK_BIT_START_X_LO (2)
#define BRICK_BIT_START_Y_LO (BRICK_BIT_START_X_LO+BRICK_BITS)
#define BRICK_BIT_START_Z_LO (BRICK_BIT_START_Y_LO+BRICK_BITS)
#define BRICK_BIT_START_X_HI (BRICK_BIT_START_Z_LO+BRICK_BITS)
#define BRICK_BIT_START_Y_HI (BRICK_BIT_START_X_HI+(BLOCK_BITS-BRICK_BITS))
#define BRICK_BIT_START_Z_HI (BRICK_BIT_START_Y_HI+(BLOCK_BITS-BRICK_BITS))

#define BRICK_BIT_SCALE_X_LO ((1<<BRICK_BIT_START_X_LO))
#define BRICK_BIT_SCALE_Y_LO ((1<<BRICK_BIT_START_Y_LO))
#define BRICK_BIT_SCALE_Z_LO ((1<<BRICK_BIT_START_Z_LO))
#define BRICK_BIT_SCALE_X_HI ((1<<BRICK_BIT_START_X_HI))
#define BRICK_BIT_SCALE_Y_HI ((1<<BRICK_BIT_START_Y_HI))
#define BRICK_BIT_SCALE_Z_HI ((1<<BRICK_BIT_START_Z_HI))
 
struct Address {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset of the voxel in the enclosing block.
  varying uint32 voxel;
};

struct Address8 {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset (in bytes!) of the voxel in the enclosing block.
  varying uint32 voxelOfs;
  varying uint32 voxelOfs_dx;
  varying uint32 voxelOfs_dy;
  varying uint32 voxelOfs_dz;
};

struct AllCornerAddresses {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset of the voxel in the enclosing block.
  varying uint32 voxel[2][2][2];
};

inline int fast_div(int x)
{
  return (int)((float)x * (1.f/(BLOCK_WIDTH-1.f)));
}

inline int fast_div(float x)
{
  return (int)((float)x * (1.f/(BLOCK_WIDTH-1.f)));
}

inline int brickTranslation(const vec3i &voxelIdxInBlock)
{
  const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;
  const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK);

  return
    (voxelIdxInBrick.x << (0*BRICK_BITS))
    | (voxelIdxInBrick.y << (1*BRICK_BITS))
    | (voxelIdxInBrick.z << (2*BRICK_BITS))
    | (brickIdxInBlock.x << (3*BRICK_BITS+0*(BLOCK_BITS-BRICK_BITS)))
    | (brickIdxInBlock.y << (3*BRICK_BITS+1*(BLOCK_BITS-BRICK_BITS)))
    | (brickIdxInBlock.z << (3*BRICK_BITS+2*(BLOCK_BITS-BRICK_BITS)))
    ;
}

inline void brickTranslation(const vec3i &voxelIdxInBlock,
                             Address8 &addr)
{
  const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;
  const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK);

  addr.voxelOfs_dx = (voxelIdxInBrick.x == (BRICK_WIDTH-1)) 
    ? +1*BRICK_BIT_SCALE_X_HI - (BRICK_WIDTH-1)*BRICK_BIT_SCALE_X_LO
    : +1*BRICK_BIT_SCALE_X_LO;
  addr.voxelOfs_dy = (voxelIdxInBrick.y == (BRICK_WIDTH-1)) 
    ? +1*BRICK_BIT_SCALE_Y_HI - (BRICK_WIDTH-1)*BRICK_BIT_SCALE_Y_LO
    : +1*BRICK_BIT_SCALE_Y_LO;
  addr.voxelOfs_dz = (voxelIdxInBrick.z == (BRICK_WIDTH-1)) 
    ? +1*BRICK_BIT_SCALE_Z_HI - (BRICK_WIDTH-1)*BRICK_BIT_SCALE_Z_LO
    : +1*BRICK_BIT_SCALE_Z_LO;

  addr.voxelOfs = 
    (voxelIdxInBrick.x << BRICK_BIT_START_X_LO) |
    (voxelIdxInBrick.y << BRICK_BIT_START_Y_LO) |
    (voxelIdxInBrick.z << BRICK_BIT_START_Z_LO) |
    (brickIdxInBlock.x << BRICK_BIT_START_X_HI) |
    (brickIdxInBlock.y << BRICK_BIT_START_Y_HI) |
    (brickIdxInBlock.z << BRICK_BIT_START_Z_HI);
}

inline void PaddedBlockBrickedVolume_getVoxelAddress8(PaddedBlockBrickedVolume *uniform volume,
                                                      const varying vec3f &voxelIdxInVolume_f,
                                                      const varying vec3i &voxelIdxInVolume,
                                                      varying Address8 &address)
{
  // Compute the 3D index of the block containing the brick containing the voxel.
  const vec3i blockIndex = make_vec3i(fast_div(voxelIdxInVolume_f.x),
                                      fast_div(voxelIdxInVolume_f.y),
                                      fast_div(voxelIdxInVolume_f.z));
  
  // Compute the 1D address of the block in the volume.
  address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  // Compute the 3D offset of the brick within the block containing the voxel.
  const vec3i voxelIdxInBlock = voxelIdxInVolume - blockIndex * make_vec3i(BLOCK_WIDTH-1);
  brickTranslation(voxelIdxInBlock,address);
}


inline void PaddedBlockBrickedVolume_getVoxelAddress(PaddedBlockBrickedVolume *uniform volume,
                                                     const varying vec3i &voxelIdxInVolume,
                                                     varying Address &address)
{
  // Compute the 3D index of the block containing the brick containing the voxel.
  const vec3i blockIndex = make_vec3i(fast_div(voxelIdxInVolume.x),
                                      fast_div(voxelIdxInVolume.y),
                                      fast_div(voxelIdxInVolume.z));
  
  // Compute the 1D address of the block in the volume.
  address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  // Compute the 3D offset of the brick within the block containing the voxel.
  const vec3i voxelIdxInBlock = voxelIdxInVolume - blockIndex * make_vec3i(BLOCK_WIDTH-1);

  const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;
  const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK);

  address.voxel
    = (voxelIdxInBrick.x << (0*BRICK_BITS))
    | (voxelIdxInBrick.y << (1*BRICK_BITS))
    | (voxelIdxInBrick.z << (2*BRICK_BITS))
    | (brickIdxInBlock.x << (3*BRICK_BITS+0*(BLOCK_BITS-BRICK_BITS)))
    | (brickIdxInBlock.y << (3*BRICK_BITS+1*(BLOCK_BITS-BRICK_BITS)))
    | (brickIdxInBlock.z << (3*BRICK_BITS+2*(BLOCK_BITS-BRICK_BITS)))
    ;
}

inline bool PaddedBlockBrickedVolume_getPaddedAddress(PaddedBlockBrickedVolume *uniform volume,
                                                      const varying vec3i &voxelIdxInVolume,
                                                      const uniform vec3i delta,
                                                      varying Address &address)
{
  // do not set offset value for original sample
  if (delta.x == 0 && delta.y == 0 && delta.z == 0)
    return false;

  // Compute the 3D index of the block containing the brick containing the voxel.
  // vec3i blockIndex = voxelIdxInVolume / make_vec3i(BLOCK_WIDTH-1);
  vec3i blockIndex = make_vec3i(fast_div(voxelIdxInVolume.x),
                                fast_div(voxelIdxInVolume.y),
                                fast_div(voxelIdxInVolume.z));
  
  // Compute the 3D offset of the brick within the block containing the voxel.
  vec3i voxelIdxInBlock = voxelIdxInVolume - blockIndex * make_vec3i(BLOCK_WIDTH-1);
  
  bool valid = false;
  if (voxelIdxInBlock.x == 0 && delta.x == 1) {
    voxelIdxInBlock.x = BLOCK_WIDTH-1;
    --blockIndex.x;
    valid = true;
  }
  if (voxelIdxInBlock.y == 0 && delta.y == 1) {
    voxelIdxInBlock.y = BLOCK_WIDTH-1;
    --blockIndex.y;
    valid = true;
  }
  if (voxelIdxInBlock.z == 0 && delta.z == 1) {
    voxelIdxInBlock.y = BLOCK_WIDTH-1;
    --blockIndex.z;
    valid = true;
  }

  // Compute the 1D address of the block in the volume.
  address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  
  const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;
  const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK);
  
  // Compute the 1D address of the voxel in the block.
  address.voxel
    = voxelIdxInBrick.x << (0*BRICK_BITS)
    | voxelIdxInBrick.y << (1*BRICK_BITS)
    | voxelIdxInBrick.z << (2*BRICK_BITS)
    | brickIdxInBlock.x << (3*BRICK_BITS-0*BLOCK_BITS)
    | brickIdxInBlock.y << (2*BRICK_BITS+1*BLOCK_BITS)
    | brickIdxInBlock.z << (1*BRICK_BITS+2*BLOCK_BITS);





  // // Compute the 3D offset of the brick within the block containing the voxel.
  // vec3i brickOffset = index - blockIndex * make_vec3i(((1<<BLOCK_WIDTH_BITCOUNT)-1));
  // if (g_dbg)
  //   print("brickOffset % % %\n",brickOffset.x,brickOffset.y,brickOffset.z);

  // if (g_dbg)
  //   print("blockIndex % % %\n",blockIndex.x,blockIndex.y,blockIndex.z);
  // if (g_dbg)
  //   print("brickOffset % % %\n",brickOffset.x,brickOffset.y,brickOffset.z);

  // // Compute the 1D address of the block in the volume.
  // address.block = blockIndex.x + volume->blockCount.x * (blockIndex.y + volume->blockCount.y * blockIndex.z);

  // // Compute the 3D offset of the voxel in the brick.
  // const vec3i voxelOffset = bitwise_AND(brickOffset, BRICK_VOXEL_BITMASK);
  // if (g_dbg)
  //   print("voxelOffset % % %\n",voxelOffset.x,voxelOffset.y,voxelOffset.z);

  // // Compute the 1D address of the brick in the block.
  // const uint32 brickAddress
  //   = brickOffset.x
  //   + (brickOffset.y << BLOCK_BRICK_WIDTH_BITCOUNT) 
  //   + (brickOffset.z << 2 * BLOCK_BRICK_WIDTH_BITCOUNT);

  // // Compute the 1D address of the voxel in the block.
  // address.voxel
  //   = brickAddress  << (3 * BRICK_BITS)
  //   | voxelOffset.z << (2 * BRICK_BITS)
  //   | voxelOffset.y << BRICK_BITS
  //   | voxelOffset.x;

  return valid && blockIndex.x >=0 && blockIndex.y >= 0 && blockIndex.z >= 0;
}

inline void PaddedBlockBrickedVolumeUChar_getVoxel(void *uniform _volume,
                                                   const varying vec3i &index,
                                                   varying float &value)
{
  // Cast to the actual Volume subtype.
  PaddedBlockBrickedVolume *uniform volume = (PaddedBlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  uint8 *uniform blockMem = (uint8 *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;
  PaddedBlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    uint8 *uniform blockPtr = blockMem + (VOXELS_PER_BLOCK * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
}

inline void PaddedBlockBrickedVolumeFloat_getVoxel(void *uniform _volume,
                                                   const varying vec3i &index,
                                                   varying float &value)
{
  // Cast to the actual Volume subtype.
  PaddedBlockBrickedVolume *uniform volume = (PaddedBlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  float *uniform blockMem = (float *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;
  PaddedBlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    float *uniform blockPtr = (float *)blockMem + (VOXELS_PER_BLOCK * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
}

inline void PaddedBlockBrickedVolumeDouble_getVoxel(void *uniform _volume,
                                                    const varying vec3i &index,
                                                    varying float &value)
{
  // Cast to the actual Volume subtype.
  PaddedBlockBrickedVolume *uniform volume = (PaddedBlockBrickedVolume *uniform) _volume;

  // Cast to the actual voxel type.
  double *uniform blockMem = (double *uniform) volume->blockMem;

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;
  PaddedBlockBrickedVolume_getVoxelAddress(volume, index, address);

  // The voxel value at the 1D address.
  foreach_unique(blockID in address.block) {
    double *uniform blockPtr = blockMem + (VOXELS_PER_BLOCK * (uint64)blockID);
    value = blockPtr[address.voxel];
  }
}

inline void PaddedBlockBrickedVolume_allocateMemory(PaddedBlockBrickedVolume *uniform volume)
{
  // Memory may already have been allocated.
  if (volume->blockMem != NULL) return;

  // Volume size in blocks per dimension with padding to the nearest block.
  volume->blockCount = (volume->super.dimensions + (BLOCK_WIDTH-1) - 1) / (BLOCK_WIDTH-1);

  // Volume size in blocks with padding.
  const uniform size_t blockCount = volume->blockCount.x * volume->blockCount.y * volume->blockCount.z;

  // allocate the large array of blocks
  uniform uint64 blockSize = VOXELS_PER_BLOCK * volume->voxelSize;
  volume->blockMem = malloc64(blockSize * (uint64)blockCount);

  if (volume->blockMem == NULL) {
    print("failed to allocate block memory!");
    return;
  }
}

task void PBBVUChar_setRegionTask(PaddedBlockBrickedVolume *uniform self,
                                  const uint8 *uniform source,
                                  const uniform vec3i &targetCoord000,
                                  const uniform vec3i &regionSize)
{
  // uniform bool dbg
  //   = targetCoord000.x == 0
  //   & targetCoord000.y == 0
  //   & targetCoord000.z == 384;

  // if (g_dbg)
  // print("target % % %\n",targetCoord000.x,targetCoord000.y,targetCoord000.z);


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

    PaddedBlockBrickedVolume_getVoxelAddress(self, coord, address);
    // if (g_dbg) print("setting base % %\n",address.block,address.voxel);
    foreach_unique(blockID in address.block) {
      uint8 *uniform blockPtr
        = ((uint8*uniform)self->blockMem) 
        + blockID * (uint64)VOXELS_PER_BLOCK;
      blockPtr[address.voxel] = run[x];
    }
    for (uniform int32 iz=0;iz<2;iz++)
      for (uniform int32 iy=0;iy<2;iy++)
        for (uniform int32 ix=0;ix<2;ix++) {
          if (PaddedBlockBrickedVolume_getPaddedAddress(self, coord, make_vec3i(ix,iy,iz),address)) {
            if (g_dbg) print("setting offset % %\n",address.block,address.voxel);
            foreach_unique(blockID in address.block) {
              uint8 *uniform blockPtr
                = ((uint8*uniform)self->blockMem) 
                + blockID * (uint64)VOXELS_PER_BLOCK;
              blockPtr[address.voxel] = run[x];
            }
          }
        }
  }
}

task void PBBVFloat_setRegionTask(PaddedBlockBrickedVolume *uniform self,
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

    PaddedBlockBrickedVolume_getVoxelAddress(self, coord, address);
    // set voxel itself
    foreach_unique(blockID in address.block) {
      float *uniform blockPtr
        = ((float*uniform)self->blockMem) 
        + blockID * (uint64)VOXELS_PER_BLOCK;
      blockPtr[address.voxel] = run[x];
    }
    // copy voxel to end of lower/left/front block if it's on the boundary
    for (uniform int32 iz=0;iz<2;iz++)
      for (uniform int32 iy=0;iy<2;iy++)
        for (uniform int32 ix=0;ix<2;ix++) {
          if (PaddedBlockBrickedVolume_getPaddedAddress(self, coord, make_vec3i(ix,iy,iz),address)) {
            foreach_unique(blockID in address.block) {
              float *uniform blockPtr
                = ((float*uniform)self->blockMem) 
                + blockID * (uint64)VOXELS_PER_BLOCK;
              blockPtr[address.voxel] = run[x];
            }
          }
        }
  }
}

task void PBBVDouble_setRegionTask(PaddedBlockBrickedVolume *uniform self,
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
    PaddedBlockBrickedVolume_getVoxelAddress(self, coord, address);
    foreach_unique(blockID in address.block) {
      double *uniform blockPtr
        = ((double*uniform)self->blockMem)
        + blockID * (uint64)VOXELS_PER_BLOCK;
      blockPtr[address.voxel] = run[x];
    }
  }
}
                                
/*! copy given block of voxels into the volume, where source[0] will
  be written to volume[targetCoord000] */
void PaddedBlockBrickedVolumeUChar_setRegion(void *uniform _volume, 
                                             const void *uniform _source, 
                                             const uniform vec3i &targetCoord000,
                                             const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] PBBVUChar_setRegionTask((PaddedBlockBrickedVolume*uniform)_volume,
                                          (const uint8*uniform)_source,
                                          targetCoord000,
                                          regionSize);
}

/*! copy given block of voxels into the volume, where source[0] will
  be written to volume[targetCoord000] */
void PaddedBlockBrickedVolumeFloat_setRegion(void *uniform _volume, 
                                             const void *uniform _source, 
                                             const uniform vec3i &targetCoord000,
                                             const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] PBBVFloat_setRegionTask((PaddedBlockBrickedVolume*uniform)_volume,
                                          (const float*uniform)_source,
                                          targetCoord000,
                                          regionSize);
}

/*! copy given block of voxels into the volume, where source[0] will
  be written to volume[targetCoord000] */
void PaddedBlockBrickedVolumeDouble_setRegion(void *uniform _volume, 
                                              const void *uniform _source, 
                                              const uniform vec3i &targetCoord000,
                                              const uniform vec3i &regionSize)
{
  // a 'run' is sequence of connected voxels in x direction 
  uniform uint32 numRuns = regionSize.y * regionSize.z;
  launch[numRuns] PBBVDouble_setRegionTask((PaddedBlockBrickedVolume*uniform)_volume,
                                           (const double*uniform)_source,
                                           targetCoord000,
                                           regionSize);
}

void PaddedBlockBrickedVolume_Constructor(PaddedBlockBrickedVolume *uniform volume, 
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
    volume->super.getVoxel = PaddedBlockBrickedVolumeUChar_getVoxel;
    volume->setRegion = &PaddedBlockBrickedVolumeUChar_setRegion;
  }
  else if (volume->voxelType == OSP_FLOAT) {
    volume->voxelSize = sizeof(uniform float);
    volume->super.getVoxel = PaddedBlockBrickedVolumeFloat_getVoxel;
    volume->setRegion = &PaddedBlockBrickedVolumeFloat_setRegion;
  }
  else if (volume->voxelType == OSP_DOUBLE) {
    volume->voxelSize = sizeof(uniform double);    
    volume->super.getVoxel = PaddedBlockBrickedVolumeDouble_getVoxel;
    volume->setRegion = &PaddedBlockBrickedVolumeDouble_setRegion;
  }
  else {
    print("#osp:block_bricked_volume: unknown voxel type\n");
    return;
  }

  // Allocate memory.
  PaddedBlockBrickedVolume_allocateMemory(volume);
}


inline float accessArrayWithOffset(const float *uniform basePtr, varying uint32 offset)
{
  uint8 *uniform base = (uint8 *uniform)basePtr;

  return *((float *)(base+offset));
}

inline varying float PaddedBlockBrickedVolume_computeSample(void *uniform _volume, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  PaddedBlockBrickedVolume *uniform volume = (PaddedBlockBrickedVolume *uniform) _volume;

  // Transform the sample location into the local coordinate system.
  vec3f localCoordinates;
  volume->super.transformWorldToLocal((StructuredVolume *uniform)volume, worldCoordinates, localCoordinates);

  // Coordinates outside the volume are clamped to the volume bounds.
  const vec3f clampedLocalCoordinates = clamp(localCoordinates, make_vec3f(0.0f), 
                                              volume->super.localCoordinatesUpperBound);

  // Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);  

  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = clampedLocalCoordinates - float_cast(voxelIndex_0);

  // Compute the 1D address of the block in the volume and the voxel in the block.
  Address8 address8;
  PaddedBlockBrickedVolume_getVoxelAddress8(volume, clampedLocalCoordinates, voxelIndex_0, address8);


  
  // TODO: this should be a specialization for 'volume < 4GB',
  // rsulting in two separate functions - one for <= 2G, one for > 2G
#if 1
  const float *uniform blockPtr = (float *uniform)volume->blockMem;
  // TODO: interleave loads with the address computations ... or at
  // least, prefetch upon the first address being known.
  const uint32 ofs000 = address8.voxelOfs + address8.block*(VOXELS_PER_BLOCK*sizeof(uniform float));
  const uint32 ofs001 = ofs000+address8.voxelOfs_dx;
  const uint32 ofs010 = ofs000+address8.voxelOfs_dy;
  const uint32 ofs011 = ofs001+address8.voxelOfs_dy;
  const uint32 ofs100 = ofs000+address8.voxelOfs_dz;
  const uint32 ofs101 = ofs001+address8.voxelOfs_dz;
  const uint32 ofs110 = ofs010+address8.voxelOfs_dz;
  const uint32 ofs111 = ofs011+address8.voxelOfs_dz;
  const float voxelValue_000 = accessArrayWithOffset(blockPtr,ofs000);
  const float voxelValue_001 = accessArrayWithOffset(blockPtr,ofs001);
  const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);

  const float voxelValue_010 = accessArrayWithOffset(blockPtr,ofs010);
  const float voxelValue_011 = accessArrayWithOffset(blockPtr,ofs011);
  const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);

  const float voxelValue_100 = accessArrayWithOffset(blockPtr,ofs100);
  const float voxelValue_101 = accessArrayWithOffset(blockPtr,ofs101);
  const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);

  const float voxelValue_110 = accessArrayWithOffset(blockPtr,ofs110);
  const float voxelValue_111 = accessArrayWithOffset(blockPtr,ofs111);
  const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  
  // Interpolate the voxel values.
  const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

#else
  // Look up the voxel values to be interpolated.
  float voxelValue_000 = 0; 
  float voxelValue_001 = 0; 
  float voxelValue_010 = 0; 
  float voxelValue_011 = 0; 
  float voxelValue_100 = 0; 
  float voxelValue_101 = 0; 
  float voxelValue_110 = 0; 
  float voxelValue_111 = 0; 

  // The voxel value at the 1D address.
  foreach_unique(blockID in address8.block) {
    const uint32 ofs000 = address8.voxelOfs;
    const uint32 dx = address8.voxelOfs_dx;
    const uint32 dy = address8.voxelOfs_dy;
    const uint32 dz = address8.voxelOfs_dz;

    const float *uniform const blockPtr = (float *uniform)volume->blockMem + (VOXELS_PER_BLOCK * (uint64)blockID);
    voxelValue_000 = accessArrayWithOffset(blockPtr,ofs000);
    voxelValue_001 = accessArrayWithOffset(blockPtr,ofs000+dx);
    voxelValue_010 = accessArrayWithOffset(blockPtr,ofs000+dy);
    voxelValue_011 = accessArrayWithOffset(blockPtr,ofs000+dx+dy);

    voxelValue_100 = accessArrayWithOffset(blockPtr,ofs000+dz);
    voxelValue_101 = accessArrayWithOffset(blockPtr,ofs000+dz+dx);
    voxelValue_110 = accessArrayWithOffset(blockPtr,ofs000+dz+dy);
    voxelValue_111 = accessArrayWithOffset(blockPtr,ofs000+dz+dx+dy);
  }
  // Interpolate the voxel values.
  const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

#endif

  return volumeSample;
}

export void *uniform PaddedBlockBrickedVolume_createInstance(void *uniform cppEquivalent,
                                                             const uniform int voxelType, 
                                                             const uniform vec3i &dimensions)
{
  // The volume container.
  PaddedBlockBrickedVolume *uniform volume = uniform new uniform PaddedBlockBrickedVolume;

  PaddedBlockBrickedVolume_Constructor(volume, cppEquivalent, voxelType, dimensions);
  if (voxelType == OSP_FLOAT) {
    print("using hand-inlined computesample fct for padded-block-bricked\n");
    // volume->super.super.computeSample = BlockBrickedVolumeFloat_computeSample;
    volume->super.super.computeSample = PaddedBlockBrickedVolume_computeSample;
  }
  return volume;
}

export void 
PaddedBlockBrickedVolume_setRegion(void *uniform _self,
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
  PaddedBlockBrickedVolume *uniform self = (PaddedBlockBrickedVolume *uniform)_self;
  self->setRegion(_self,_source,regionCoords,regionSize);
}
