// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "ospray/volume/GhostBlockBrickedVolume.ih"

/*! total number of bits per block dimension. '6' would mean 18 bits =
  1/4million voxels per block, which for alots would be 1MB, so should
  all fit into a single 2M page (assuming we do 2M allocs, of course) */
#define BLOCK_BITS (6)
#define BRICK_BITS (2)

/*! @{ number of voxels per block per dimension */
#define BLOCK_WIDTH (1<<BLOCK_BITS)
#define BRICK_WIDTH (1<<BRICK_BITS)
/*! @} */

/*! bit mask for extracting the _brick_ bits from a given per-block
  coordinate */
#define BRICK_MASK (BRICK_WIDTH-1)

/*! size of one block, in voxels */
#define VOXELS_PER_BLOCK (BLOCK_WIDTH*BLOCK_WIDTH*BLOCK_WIDTH)

/*! @{ start bit for given section of address in bricked offset field

  lo is the address part for inside a brick, hi is the remaining part
  inside the blcok ex the brick part
*/
#define BRICK_BIT_X_LO (0)
#define BRICK_BIT_Y_LO (BRICK_BIT_X_LO+BRICK_BITS)
#define BRICK_BIT_Z_LO (BRICK_BIT_Y_LO+BRICK_BITS)
#define BRICK_BIT_X_HI (BRICK_BIT_Z_LO+BRICK_BITS)
#define BRICK_BIT_Y_HI (BRICK_BIT_X_HI+(BLOCK_BITS-BRICK_BITS))
#define BRICK_BIT_Z_HI (BRICK_BIT_Y_HI+(BLOCK_BITS-BRICK_BITS))
/* @} */

/*! @{ how much we have to sacle a given linear coordinate inside a
  brick to end up at the proper bit fields required by the brick
  translation */
#define BRICK_BIT_SCALE_X_LO ((1<<BRICK_BIT_X_LO))
#define BRICK_BIT_SCALE_Y_LO ((1<<BRICK_BIT_Y_LO))
#define BRICK_BIT_SCALE_Z_LO ((1<<BRICK_BIT_Z_LO))
#define BRICK_BIT_SCALE_X_HI ((1<<BRICK_BIT_X_HI))
#define BRICK_BIT_SCALE_Y_HI ((1<<BRICK_BIT_Y_HI))
#define BRICK_BIT_SCALE_Z_HI ((1<<BRICK_BIT_Z_HI))
/* @} */

/*! @{ number of bits we have to SHIFT addresses for given type */
#define shift_per_uint8 0
#define shift_per_uint16 1
#define shift_per_float 2
#define shift_per_double 3
/*! @} */

/*! @{ number of bits we have to MULTIPLY offsets with for given
type */
#define scale_per_uint8 1
#define scale_per_uint16 2
#define scale_per_float 4
#define scale_per_double 8
/*! @} */


/*! do a simple type rename/abbreviation, to we have shorter type names */
typedef GhostBlockBrickedVolume GBBV;

/*! structure that encodes a block:offset style address for a single
  sample */
struct Address {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset of the voxel in the enclosing block.
  varying uint32 voxel;
};


/*! struct that encodes all 8 corners in a (ghosted) block by block
  ID, base of the lower-left, and offset's to the +1's in x,y,and
  z. Deltas fully contain the effet of bricking */
struct Address8 {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset (in bytes!) of the voxel in the enclosing block.
  varying uint32 voxelOfs;
  /*! the offset we have to add to voxelofs to go to the next voxel in
    x direction (only works for _one_ step in x, not multiple */
  varying uint32 voxelOfs_dx;
  /*! the offset we have to add to voxelofs to go to the next voxel in
    y direction (only works for _one_ step in y, not multiple */
  varying uint32 voxelOfs_dy;
  /*! the offset we have to add to voxelofs to go to the next voxel in
    z direction (only works for _one_ step in z, not multiple */
  varying uint32 voxelOfs_dz;
};

/*! fast 32-bit integer division via floats */
inline int intDiv(int x)
// { return ((int)x) / (BLOCK_WIDTH-1.f); }
{ return (int)((float)x * (1.f/(BLOCK_WIDTH-1.f))); }

/*! fast 32-bit integer division via floats; for the case where we
  already have a float-version of the original integer */
inline int intDiv(float x)
// { return ((int)x) / (BLOCK_WIDTH-1.f); }
{ return (int)((float)x * (1.f/(BLOCK_WIDTH-1.f))); }

#define template_brickTranslation(type)                                 \
  /*! compute address8 for a given voxel index, including bricking */   \
  inline void brickTranslation_##type(const vec3i &voxelIdxInBlock,     \
                                      Address8 &addr)                   \
  {                                                                     \
    const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;        \
    const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK); \
                                                                        \
    addr.voxelOfs_dx = (voxelIdxInBrick.x == (BRICK_WIDTH-1))           \
      ?                                                                 \
      + (BRICK_BIT_SCALE_X_HI*scale_per_##type)                         \
      - ((BRICK_WIDTH-1)*BRICK_BIT_SCALE_X_LO*scale_per_##type)         \
      : (BRICK_BIT_SCALE_X_LO*scale_per_##type);                        \
                                                                        \
    addr.voxelOfs_dy = (voxelIdxInBrick.y == (BRICK_WIDTH-1))           \
      ?                                                                 \
      + (BRICK_BIT_SCALE_Y_HI*scale_per_##type)                         \
      - ((BRICK_WIDTH-1)*BRICK_BIT_SCALE_Y_LO*scale_per_##type)         \
      : (BRICK_BIT_SCALE_Y_LO*scale_per_##type);                        \
                                                                        \
    addr.voxelOfs_dz = (voxelIdxInBrick.z == (BRICK_WIDTH-1))           \
      ?                                                                 \
      + (BRICK_BIT_SCALE_Z_HI*scale_per_##type)                         \
      - ((BRICK_WIDTH-1)*BRICK_BIT_SCALE_Z_LO*scale_per_##type)         \
      : (BRICK_BIT_SCALE_Z_LO*scale_per_##type);                        \
                                                                        \
    addr.voxelOfs =                                                     \
      (voxelIdxInBrick.x << (BRICK_BIT_X_LO+shift_per_##type)) |        \
      (voxelIdxInBrick.y << (BRICK_BIT_Y_LO+shift_per_##type)) |        \
      (voxelIdxInBrick.z << (BRICK_BIT_Z_LO+shift_per_##type)) |        \
      (brickIdxInBlock.x << (BRICK_BIT_X_HI+shift_per_##type)) |        \
      (brickIdxInBlock.y << (BRICK_BIT_Y_HI+shift_per_##type)) |        \
      (brickIdxInBlock.z << (BRICK_BIT_Z_HI+shift_per_##type));         \
         }

template_brickTranslation(uint8);
template_brickTranslation(uint16);
template_brickTranslation(float);
template_brickTranslation(double);

#define template_getAddress(type)                                       \
  /* IMPORTANT: this function computes RAW adresses that are to be */   \
  /* directy added to "unsigned char *" pointers */                     \
  inline void GBBV_getAddress_##type(GBBV *uniform volume,              \
                                     const vec3f &voxelIdxInVolume_f,   \
                                     const vec3i &voxelIdxInVolume,     \
                                     varying Address8 &address)         \
  {                                                                     \
    /* Compute the 3D index of the block containing the brick containing the voxel. */ \
    const vec3i blockIndex = make_vec3i(intDiv(voxelIdxInVolume_f.x),   \
                                        intDiv(voxelIdxInVolume_f.y),   \
                                        intDiv(voxelIdxInVolume_f.z));  \
                                                                        \
    /* Compute the 1D address of the block in the volume. */            \
    address.block                                                       \
      = blockIndex.x                                                    \
      + volume->blockCount.x * (blockIndex.y                            \
                                + volume->blockCount.y * blockIndex.z); \
                                                                        \
    /* Compute the 3D offset of the brick within the block containing the voxel. */ \
    const vec3i voxelIdxInBlock                                         \
      = voxelIdxInVolume - blockIndex * make_vec3i(BLOCK_WIDTH-1);      \
    brickTranslation_##type(voxelIdxInBlock,address);                   \
  }                                                                     \
                                                                        \
  /* IMPORTANT: this function computes type-relative offsets, so  */    \
  /* values returned from this function should only be added to */      \
  /* "type *" pointers */                                               \
  inline void GBBV_getIndices_##type(GBBV *uniform volume,              \
                                     const varying vec3i &voxelIdxInVolume, \
                                     varying Address &address)          \
  {                                                                     \
    /* Compute the 3D index of the block containing the brick containing the voxel. */ \
    const vec3i blockIndex = make_vec3i(intDiv(voxelIdxInVolume.x),     \
                                        intDiv(voxelIdxInVolume.y),     \
                                        intDiv(voxelIdxInVolume.z));    \
                                                                        \
    /* Compute the 1D address of the block in the volume. */            \
    address.block                                                       \
      = blockIndex.x                                                    \
      + volume->blockCount.x * (blockIndex.y                            \
                                + volume->blockCount.y * blockIndex.z); \
                                                                        \
    /* Compute the 3D offset of the brick within the block containing the voxel. */ \
    const vec3i voxelIdxInBlock                                         \
      = voxelIdxInVolume                                                \
      - blockIndex * make_vec3i(BLOCK_WIDTH-1);                         \
                                                                        \
    const vec3i brickIdxInBlock                                         \
      = voxelIdxInBlock >> BRICK_BITS;                                  \
    const vec3i voxelIdxInBrick                                         \
      = bitwise_AND(voxelIdxInBlock,BRICK_MASK);                        \
                                                                        \
    address.voxel                                                       \
      = (voxelIdxInBrick.x << (0*BRICK_BITS))                           \
      | (voxelIdxInBrick.y << (1*BRICK_BITS))                           \
      | (voxelIdxInBrick.z << (2*BRICK_BITS))                           \
      | (brickIdxInBlock.x << (3*BRICK_BITS+0*(BLOCK_BITS-BRICK_BITS))) \
      | (brickIdxInBlock.y << (3*BRICK_BITS+1*(BLOCK_BITS-BRICK_BITS))) \
      | (brickIdxInBlock.z << (3*BRICK_BITS+2*(BLOCK_BITS-BRICK_BITS))) \
      ;                                                                 \
  }

template_getAddress(uint8);
template_getAddress(uint16);
template_getAddress(float);
template_getAddress(double);

/*! since we do ghosting some cells have to be written twice. the way
    we do that is that every time we write a given cell x/y/z we check
    if this is anywhere on the front/bottom/left of a block, and if
    so, tell the setRegion call that there'll be another cell in some
    other abuttin gblock that should get this same value. what _this_
    function does is that, given a cell ID x/y/z and a 'direction' we
    have to check for ghosting (ie, on the left, on the bottom, on the
    front, or any combination thereof), determine if this cell does
    have a ghost _in that direction_, and if so, return true and
    compute the porper address for that ghost cell (in only that
    direction, mind!). if there is not ghost cell (in this direction)
    the function returns false. Thus, all the setRegion has to do is
    call 'getGhostIndices()' for all 7 possible directions that a cell
    might have ghost cells. Yes, there's faster ways to do that -
    because certain values will get recomputed for each of the 7
    direction (plus the original cell itself) but this is the easiest
    (I could find). */
inline bool GBBV_getGhostIndices(GBBV *uniform volume,
                                 const varying vec3i &voxelIdxInVolume,
                                 const uniform vec3i delta,
                                 varying Address &address)
{
  // do not set offset value for original sample
  if ((delta.x == 0) && (delta.y == 0) && (delta.z == 0))
    return false;

  // Compute the 3D index of the block containing the brick containing the voxel.
  // vec3i blockIndex = voxelIdxInVolume / make_vec3i(BLOCK_WIDTH-1);
  vec3i blockIndex = make_vec3i(intDiv(voxelIdxInVolume.x),
                                intDiv(voxelIdxInVolume.y),
                                intDiv(voxelIdxInVolume.z));
  vec3i oldBlockIndex = blockIndex;;

  // Compute the 3D offset of the brick within the block containing the voxel.
  vec3i voxelIdxInBlock = voxelIdxInVolume - blockIndex * make_vec3i(BLOCK_WIDTH-1);
  vec3i oldIdxInBlock = voxelIdxInBlock;

  bool valid = false;
  if ((voxelIdxInBlock.x == 0) && (delta.x == 1)) {
    voxelIdxInBlock.x = BLOCK_WIDTH-1;
    --blockIndex.x;
    valid = true;
  }
  if ((voxelIdxInBlock.y == 0) && (delta.y == 1)) {
    voxelIdxInBlock.y = BLOCK_WIDTH-1;
    --blockIndex.y;
    valid = true;
  }
  if ((voxelIdxInBlock.z == 0) && (delta.z == 1)) {
    voxelIdxInBlock.z = BLOCK_WIDTH-1;
    --blockIndex.z;
    valid = true;
  }

  // Compute the 1D address of the block in the volume.
  address.block
    = blockIndex.x
    + volume->blockCount.x * (blockIndex.y
                              + volume->blockCount.y * blockIndex.z);

  const vec3i brickIdxInBlock = voxelIdxInBlock >> BRICK_BITS;
  const vec3i voxelIdxInBrick = bitwise_AND(voxelIdxInBlock,BRICK_MASK);

  // Compute the 1D address of the voxel in the block.
  address.voxel =
    (voxelIdxInBrick.x << (BRICK_BIT_X_LO)) |
    (voxelIdxInBrick.y << (BRICK_BIT_Y_LO)) |
    (voxelIdxInBrick.z << (BRICK_BIT_Z_LO)) |
    (brickIdxInBlock.x << (BRICK_BIT_X_HI)) |
    (brickIdxInBlock.y << (BRICK_BIT_Y_HI)) |
    (brickIdxInBlock.z << (BRICK_BIT_Z_HI));

  return valid && (blockIndex.x >=0) && (blockIndex.y >= 0) && (blockIndex.z >= 0);
}

/*! read a _typed_ value from an address that's given by an
    *BYTE*-offset relative to a base array. note that even though we
    assume that the offset is already in bytes (ie, WITHOUT scaling by
    the width of the array data type), the the base pointer must
    *STILL* be of the proper type for this macro to figure out the
    type of data it's reading from this address */
#define template_accessArray(type)                                      \
  inline float accessArrayWithOffset(const type *uniform basePtr,       \
                                     const varying uint32 offset)       \
  {                                                                     \
    uniform uint8 *uniform base = (uniform uint8 *uniform)basePtr;      \
    return *((uniform type *)(base+offset));                            \
  }                                                                     \

template_accessArray(uint8);
template_accessArray(uint16);
template_accessArray(float);
template_accessArray(double);
#undef template_accessArray

#define template_getVoxel(type)                                         \
  /*! get a single voxel. careful - THIS function operates on */        \
  /* typed offsets, not byte-offsets */                                 \
  inline void GBBV_getVoxel_##type(void *uniform _volume,               \
                                   const varying vec3i &index,          \
                                   varying float &value)                \
  {                                                                     \
    /* Cast to the actual Volume subtype. */                            \
    GBBV *uniform volume = (GBBV *uniform) _volume;                     \
                                                                        \
    /* Cast to the actual voxel type.*/                                 \
    const type *uniform blockMem                                        \
      = (const type *uniform) volume->blockMem;                         \
                                                                        \
    /* Compute the 1D address of the block in the volume and the voxel in the block. */ \
    Address address;                                                    \
    GBBV_getIndices_##type(volume, index, address);                     \
                                                                        \
    /* The voxel value at the 1D address.*/                             \
    foreach_unique(blockID in address.block) {                          \
      const type *uniform blockPtr                                      \
        = blockMem + (VOXELS_PER_BLOCK * (uint64)blockID);              \
      value = blockPtr[address.voxel];                                  \
      /* value = accessArrayWithOffset(blockPtr,address.voxel); */      \
    }                                                                   \
  }

template_getVoxel(uint8)
template_getVoxel(uint16)
template_getVoxel(float)
template_getVoxel(double)
#undef template_getVoxel

#define template_setRegion(type)                                        \
  void GBBV_setRegionTask_##type(GBBV *uniform self,                    \
                                 const void *uniform _source,           \
                                 const uniform vec3i &targetCoord000,   \
                                 const uniform vec3i &regionSize,       \
                                 const uniform int taskIndex)           \
  {                                                                     \
    const type *uniform source = (const type *uniform)_source;          \
    const uniform uint32 region_y = taskIndex % regionSize.y;           \
    const uniform uint32 region_z = taskIndex / regionSize.y;           \
    const uniform uint64 runOfs                                         \
      = (uint64)regionSize.x                                            \
      * (region_y + (uint64)regionSize.y * region_z);                   \
    const type *uniform run = source + runOfs;                          \
                                                                        \
    vec3i coord = targetCoord000 + make_vec3i(0,region_y,region_z);     \
    foreach (x = 0 ... regionSize.x) {                                  \
      Address address;                                                  \
      coord.x = targetCoord000.x + x;                                   \
      if (coord.x < 0 ||                                                \
          coord.y < 0 ||                                                \
          coord.z < 0 ||                                                \
          coord.x >= self->super.dimensions.x ||                        \
          coord.y >= self->super.dimensions.y ||                        \
          coord.z >= self->super.dimensions.z                           \
          )                                                             \
        continue;                                                       \
                                                                        \
      GBBV_getIndices_##type(self, coord, address);                     \
      /* set voxel itself */                                            \
      foreach_unique(blockID in address.block) {                        \
        type *uniform blockPtr                                          \
          = ((type*uniform)self->blockMem)                              \
          + blockID * (uint64)VOXELS_PER_BLOCK;                         \
        blockPtr[address.voxel] = run[x];                               \
      }                                                                 \
      /* copy voxel to end of lower/left/front block if it's on the boundary */ \
      for (uniform int32 iz=0;iz<2;iz++)                                \
        for (uniform int32 iy=0;iy<2;iy++)                              \
          for (uniform int32 ix=0;ix<2;ix++) {                          \
            if (GBBV_getGhostIndices(self, coord,                       \
                                     make_vec3i(ix,iy,iz),address)) {   \
              foreach_unique(blockID in address.block) {                \
                type *uniform blockPtr                                  \
                  = ((type*uniform)self->blockMem)                      \
                  + (uint64)blockID * (uint64)VOXELS_PER_BLOCK;         \
                blockPtr[address.voxel] = run[x];                       \
              }                                                         \
            }                                                           \
          }                                                             \
    }                                                                   \
  }

template_setRegion(uint8)
template_setRegion(uint16)
template_setRegion(float)
template_setRegion(double)
#undef template_setRegion

/*! perform trilinear interpolation for given sample. unlike old way
  of doing this (a single computesample on the StructuredVolume level
  that calls the virtual 'getSample()' of the volume layout) this
  function will directly do all the addressing for the getSample
  (inlined), and thus be about 50% faster (wall-time, meaning even
  much faster in pure sample speed) */
#define template_computeSample(type)                                    \
  inline float GBBV_computeSample_##type(void *uniform _volume,         \
                                         const vec3f &worldCoordinates) \
  {                                                                     \
    /* Cast to the actual Volume subtype. */                            \
    GBBV *uniform volume = (GBBV *uniform) _volume;                     \
                                                                        \
    /* Transform the sample location into the local coordinate system. */ \
    vec3f localCoordinates;                                             \
    volume->super.transformWorldToLocal(&volume->super,                 \
                                        worldCoordinates,               \
                                        localCoordinates);              \
                                                                        \
    /* Coordinates outside the volume are clamped to the volume bounds. */ \
    const vec3f clampedLocalCoordinates                                 \
      = clamp(localCoordinates, make_vec3f(0.0f),                       \
              volume->super.localCoordinatesUpperBound);                \
                                                                        \
    /* Lower and upper corners of the box straddling the voxels to be interpolated. */ \
    const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);   \
                                                                        \
    /* Fractional coordinates within the lower corner voxel used during interpolation. */ \
    const vec3f frac = clampedLocalCoordinates - float_cast(voxelIndex_0); \
                                                                        \
    /* Compute the 1D address of the block in the volume and the voxel in the block. */ \
    Address8 address8;                                                  \
    GBBV_getAddress_##type(volume, clampedLocalCoordinates,             \
                           voxelIndex_0, address8);                     \
      /* split block into 24+8 bits; we'll have to do a foreach_unique */ \
      /* over the upper 24 bits to satify 64-bit addressing, but at */  \
      /* least the lower bits we can merge into a single loop */        \
      const uint32 block_lo = address8.block & 255;                     \
      const uint32 block_hi = address8.block ^ block_lo;                \
                                                                        \
      float retValue;                                                   \
                                                                        \
      foreach_unique (unique_block_hi in block_hi) {                    \
        const type *uniform blockPtrHi                                  \
          = (const type *uniform)volume->blockMem                       \
          + ((uniform uint64)unique_block_hi)                           \
          * (VOXELS_PER_BLOCK);                                         \
        /* TODO: interleave loads with the address computations ... or at */ \
        /* least, prefetch upon the first address being known. */       \
        const uint32 ofs000 = address8.voxelOfs                         \
          + block_lo*(VOXELS_PER_BLOCK*scale_per_##type);               \
        const uint32 ofs001 = ofs000+address8.voxelOfs_dx;              \
        const float val000  = accessArrayWithOffset(blockPtrHi,ofs000); \
        const float val001  = accessArrayWithOffset(blockPtrHi,ofs001); \
        const float val00   = val000 + frac.x * (val001 - val000);      \
                                                                        \
        const uint32 ofs010 = ofs000+address8.voxelOfs_dy;              \
        const uint32 ofs011 = ofs001+address8.voxelOfs_dy;              \
        const float val010  = accessArrayWithOffset(blockPtrHi,ofs010); \
        const float val011  = accessArrayWithOffset(blockPtrHi,ofs011); \
        const float val01   = val010 + frac.x * (val011 - val010);      \
                                                                        \
        const uint32 ofs100 = ofs000+address8.voxelOfs_dz;              \
        const uint32 ofs101 = ofs001+address8.voxelOfs_dz;              \
        const float val100  = accessArrayWithOffset(blockPtrHi,ofs100); \
        const float val101  = accessArrayWithOffset(blockPtrHi,ofs101); \
        const float val10   = val100 + frac.x * (val101 - val100);      \
                                                                        \
        const uint32 ofs110 = ofs010+address8.voxelOfs_dz;              \
        const uint32 ofs111 = ofs011+address8.voxelOfs_dz;              \
        const float val110  = accessArrayWithOffset(blockPtrHi,ofs110); \
        const float val111  = accessArrayWithOffset(blockPtrHi,ofs111); \
        const float val11   = val110 + frac.x * (val111 - val110);      \
                                                                        \
        /* Interpolate the voxel values. */                             \
        const float val0    = val00  + frac.y * (val01  - val00 );      \
        const float val1    = val10  + frac.y * (val11  - val10 );      \
        const float val     = val0   + frac.z * (val1   - val0  );      \
                                                                        \
        retValue = val;                                                 \
      }                                                                 \
      return retValue;                                                  \
  }

template_computeSample(uint8)
template_computeSample(uint16)
template_computeSample(float)
template_computeSample(double)
#undef template_computeSample


inline void GBBV_allocateMemory(GBBV *uniform volume)
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

void GBBV_Constructor(GBBV *uniform volume,
                      /*! pointer to the c++-equivalent class of this entity */
                      void *uniform cppEquivalent,
                      const uniform int voxelType,
                      const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&volume->super, cppEquivalent, dimensions);

  volume->blockMem = NULL;
  volume->voxelType = (OSPDataType) voxelType;

  if (volume->voxelType == OSP_UCHAR) {
    volume->voxelSize      = sizeof(uniform uint8);
    volume->super.getVoxel = GBBV_getVoxel_uint8;
    volume->setRegion      = &GBBV_setRegionTask_uint8;
    volume->super.super.computeSample = GBBV_computeSample_uint8;
  }
  else if (volume->voxelType == OSP_USHORT) {
    volume->voxelSize      = sizeof(uniform uint16);
    volume->super.getVoxel = GBBV_getVoxel_uint16;
    volume->setRegion      = &GBBV_setRegionTask_uint16;
    volume->super.super.computeSample = GBBV_computeSample_uint16;
  }
  else if (volume->voxelType == OSP_FLOAT) {
    volume->voxelSize      = sizeof(uniform float);
    volume->super.getVoxel = GBBV_getVoxel_float;
    volume->setRegion      = &GBBV_setRegionTask_float;
    volume->super.super.computeSample = GBBV_computeSample_float;
  }
  else if (volume->voxelType == OSP_DOUBLE) {
    volume->voxelSize      = sizeof(uniform double);
    volume->super.getVoxel = GBBV_getVoxel_double;
    volume->setRegion      = &GBBV_setRegionTask_double;
    volume->super.super.computeSample = GBBV_computeSample_double;
  }
  else {
    print("#osp:block_bricked_volume: unknown voxel type\n");
    return;
  }

  // Allocate memory.
  GBBV_allocateMemory(volume);
}

export void *uniform GBBV_createInstance(void *uniform cppEquivalent,
                                         const uniform int voxelType,
                                         const uniform vec3i &dimensions)
{
  // create instance of actual volume type ...
  GBBV *uniform volume = uniform new uniform GBBV;
  // ... call constructor to initialize it ...
  GBBV_Constructor(volume, cppEquivalent, voxelType, dimensions);
  // ... and return object to caller
  return volume;
}

export void GBBV_setRegion(void *uniform _self,
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
  GBBV *uniform self = (GBBV *uniform)_self;
  self->setRegion(self, _source, regionCoords, regionSize, taskIndex);
}

export void GBBV_freeVolume(void *uniform _self)
{
  GBBV *uniform self = (GBBV *uniform)_self;
  if (self->blockMem) {
    free64(self->blockMem);
  }
}
