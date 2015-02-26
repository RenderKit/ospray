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

//! Transform the given point into world coordinates.
vec3f BlockBrickedVolume_transformLocalToWorld(void *uniform pointer, const varying vec3f &coordinates);

//! Transform the given point into the local coordinate system of the volume.
vec3f BlockBrickedVolume_transformWorldToLocal(void *uniform pointer, const varying vec3f &coordinates);

struct Address {

  //! The 1D address of the block in the volume containing the voxel.
  varying uint32 block;

  //! The 1D offset of the voxel in the enclosing block.
  varying uint32 voxel;

};

varying vec3f BlockBrickedVolume_computeGradient(void *uniform pointer, const varying vec3f &location) {

  //! Cast to the actual Volume type.
  Volume *uniform volume = (Volume *uniform) pointer;

  //! Transform the sample location into the local coordinate system.
  const vec3f coordinates = BlockBrickedVolume_transformWorldToLocal(pointer, location);

  //! The gradient will be computed using central differences.
  varying vec3f gradient;

  //! Gradient magnitude in the X direction.
  gradient.x = volume->computeSample(volume, coordinates + make_vec3f(1.0f, 0.0f, 0.0f)) - volume->computeSample(volume, coordinates + make_vec3f(-1.0f, 0.0f, 0.0f));

  //! Gradient magnitude in the Y direction.
  gradient.y = volume->computeSample(volume, coordinates + make_vec3f(0.0f, 1.0f, 0.0f)) - volume->computeSample(volume, coordinates + make_vec3f(0.0f, -1.0f, 0.0f));

  //! Gradient magnitude in the Z direction.
  gradient.z = volume->computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f, 1.0f)) - volume->computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f, -1.0f));

  //! This approximation may yield image artifacts.
  return(gradient * 0.5f);

}

varying float BlockBrickedVolume_computeSample(void *uniform pointer, const varying vec3f &location) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Transform the sample location into the local coordinate system.
  const vec3f coordinates = BlockBrickedVolume_transformWorldToLocal(pointer, location);

  //! Coordinates outside the volume are clamped to the volume bounds.
  const vec3f clampedCoordinates = clamp(coordinates, make_vec3f(0.0f), volume->inherited.upperBound);

  //! Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = integer_cast(clampedCoordinates);  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  //! Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalCoordinates = clampedCoordinates - float_cast(voxelIndex_0);

  //! Look up the voxel values to be interpolated.
  const float voxelValue_000 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
  const float voxelValue_001 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
  const float voxelValue_010 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
  const float voxelValue_011 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
  const float voxelValue_100 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
  const float voxelValue_101 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
  const float voxelValue_110 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
  const float voxelValue_111 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));

  //! Interpolate the voxel values.
  const float voxelValue_00 = voxelValue_000 + fractionalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 = voxelValue_010 + fractionalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 = voxelValue_100 + fractionalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 = voxelValue_110 + fractionalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0  = voxelValue_00  + fractionalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalCoordinates.z * (voxelValue_1   - voxelValue_0  );
  return(volumeSample);

}

inline void BlockBrickedVolume_getVoxelAddress(BlockBrickedVolume *uniform volume, const varying vec3i &index, varying Address &address) {

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

inline varying float BlockBrickedVolumeFloat_getVoxel(void *uniform pointer, const varying vec3i &index) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Cast to the actual voxel type.
  float **uniform voxelData = (float **uniform) volume->voxelData;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  //! The voxel value at the 1D address.
  float value;  foreach_unique(block in address.block) value = voxelData[block][address.voxel];  return(value);

}

inline varying float BlockBrickedVolumeUChar_getVoxel(void *uniform pointer, const varying vec3i &index) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Cast to the actual voxel type.
  uint8 **uniform voxelData = (uint8 **uniform) volume->voxelData;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index, address);

  //! The voxel value at the 1D address.
  float value;  foreach_unique(block in address.block) value = voxelData[block][address.voxel];  return(value / 255.0f);

}

void BlockBrickedVolume_intersect(void *uniform pointer, varying Ray &ray) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! The recommended step size for ray casting based volume renderers.
  const uniform float step = 1.0f / reduce_max(volume->volumeDimensions) / volume->inherited.samplingRate;

  //! Compute the next hit point using a spatial acceleration structure.
  GridAccelerator_intersect(volume->accelerator, step, ray);
//ray.t += step;

}

inline void BlockBrickedVolumeFloat_setVoxel(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Cast to the actual voxel type.
  float **uniform voxelData = (float **uniform) volume->voxelData;  const float *uniform sourceData = (const float *uniform) source;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

  //! The source voxel value.
  float value = sourceData[(offset.z * volume->volumeDimensions.y + offset.y) * volume->volumeDimensions.x + offset.x];

  //! Store the voxel value at the 1D address.
  foreach_unique(block in address.block) voxelData[block][address.voxel] = value;

}

inline void BlockBrickedVolumeUChar_setVoxel(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Cast to the actual voxel type.
  uint8 **uniform voxelData = (uint8 **uniform) volume->voxelData;  const uint8 *uniform sourceData = (const uint8 *uniform) source;

  //! Compute the 1D address of the block in the volume and the voxel in the block.
  Address address;  BlockBrickedVolume_getVoxelAddress(volume, index + offset, address);

  //! The source voxel value.
  uint8 value = sourceData[(offset.z * volume->volumeDimensions.y + offset.y) * volume->volumeDimensions.x + offset.x];

  //! Store the voxel value at the 1D address.
  foreach_unique(block in address.block) voxelData[block][address.voxel] = value;

}

inline vec3f BlockBrickedVolume_transformLocalToWorld(void *uniform pointer, const varying vec3f &coordinates) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Transform the point into the world coordinate system.
  return(coordinates / volume->inherited.upperBound);

}

inline vec3f BlockBrickedVolume_transformWorldToLocal(void *uniform pointer, const varying vec3f &coordinates) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Transform the point into the local coordinate system.
  return(coordinates * volume->inherited.upperBound);

}

export void BlockBrickedVolume_allocateMemory(void *uniform pointer) {

  //! The ISPC compiler fails during allocation of pointer types.
  typedef void *uniform Block;

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Memory may already have been allocated.
  if (volume->voxelData != NULL) return;

  //! Volume size in blocks with padding.
  const uniform size_t blockCount = volume->blockCount.x * volume->blockCount.y * volume->blockCount.z;

  //! Allocate storage for pointers to the voxel blocks.
  volume->voxelData = uniform new uniform Block[blockCount];  if (volume->voxelData == NULL) return;

  //! Allocate storage for the individual voxel blocks.
  for (uniform size_t i=0 ; i < blockCount ; i++) volume->voxelData[i] = (void *uniform)(uniform new uniform uint8[BLOCK_VOXEL_COUNT * volume->voxelSize]);

}

export void *uniform BlockBrickedVolume_createInstance(const uniform int voxelType) {

  //! The volume container.
  BlockBrickedVolume *uniform volume = uniform new uniform BlockBrickedVolume;

  //! Denote that the acceleration structure does not exist yet.
  volume->accelerator = NULL;

  //! Store the voxel type.
  volume->voxelType = (OSPDataType) voxelType;

  //! Store the voxel size.
  volume->voxelSize = (volume->voxelType == OSP_FLOAT) ? sizeof(uniform float) : sizeof(uniform uint8);

  //! Function to compute the gradient at a sample location in the volume.
  volume->inherited.computeGradient = BlockBrickedVolume_computeGradient;

  //! Function to sample the volume.
  volume->inherited.computeSample = BlockBrickedVolume_computeSample;

  //! Function to find the next hit point for ray casting based renderers.
  volume->inherited.intersect = BlockBrickedVolume_intersect;

  //! Function to get a voxel value.
  volume->getVoxel = (volume->voxelType == OSP_FLOAT) ? BlockBrickedVolumeFloat_getVoxel : BlockBrickedVolumeUChar_getVoxel;

  //! Function to set a voxel value.
  volume->setVoxel = (volume->voxelType == OSP_FLOAT) ? BlockBrickedVolumeFloat_setVoxel : BlockBrickedVolumeUChar_setVoxel;

  //! Indicate that memory allocation is needed.
  volume->voxelData = NULL;  return(volume);

}

export void BlockBrickedVolume_finish(void *uniform pointer) {

  //! Cast to the actual Volume type.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the accelerator structure field.
  volume->accelerator = GridAccelerator_createInstance(pointer);

}

export void BlockBrickedVolume_setGammaCorrection(void *uniform pointer, const uniform vec2f &value) {

  //! Cast to the actual Volume type.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the gamma correction coefficient and exponent.
  volume->inherited.gammaCorrection = value;

}

export void BlockBrickedVolume_setRegion(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const uniform vec3i &count) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Copy voxel data from memory into the volume.
  foreach (z = 0 ... count.z, y = 0 ... count.y, x = 0 ... count.x) { const vec3i offset = make_vec3i(x, y, z);  volume->setVoxel(volume, source, index, offset); }

}

export void BlockBrickedVolume_setSamplingRate(void *uniform pointer, const uniform float value) {

  //! Cast to the actual Volume type.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the recommended sampling rate for ray casting based renderers.
  volume->inherited.samplingRate = value;

}

export void BlockBrickedVolume_setTransferFunction(void *uniform pointer, void *uniform value) {

  //! Cast to the actual Volume type.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the color and opacity transfer function.
  volume->inherited.transferFunction = (TransferFunction *uniform) value;

}

export void BlockBrickedVolume_setValueRange(void *uniform pointer, const uniform vec2f &value) {

  //! Cast to the actual Volume type.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the volume value range.
  volume->inherited.valueRange = value;

}

export void BlockBrickedVolume_setVolumeDimensions(void *uniform pointer, const uniform vec3i &value) {

  //! Cast to the actual Volume subtype.
  BlockBrickedVolume *uniform volume = (BlockBrickedVolume *uniform) pointer;

  //! Set the step size for ray casting based renderers.
  volume->volumeDimensions = value;

  //! The largest floating point value still inside the volume.
  volume->inherited.upperBound = nextafter(volume->volumeDimensions - 1, make_vec3i(0));

  //! Volume size in blocks per dimension with padding to the nearest block.
  volume->blockCount = (volume->volumeDimensions + BLOCK_VOXEL_WIDTH - 1) / BLOCK_VOXEL_WIDTH;

}

