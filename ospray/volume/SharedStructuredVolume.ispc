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

#include "ospray/volume/SharedStructuredVolume.ih"

// -----------------------------------------------------------------------------
// versions for pure 32-bit addressing. volume *MUST* be smaller than 2G
// -----------------------------------------------------------------------------

/*! get a voxel from given volume type, assuming we can use 32-bit addressing */
inline void SharedStructuredVolumeFloat_getVoxel_32(void *uniform _volume, 
                                                    const varying vec3i &index, 
                                                    varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  // Cast to the actual voxel type.
  const float *uniform voxelData = (const float *uniform) volume->voxelData;
  const uint32 addr = index.x + volume->super.dimensions.x * (index.y + volume->super.dimensions.y * index.z);

  // The voxel value at the given index.
  value = voxelData[addr];
}

/*! get a voxel from given volume type, assuming we can use 32-bit addressing */
inline void SharedStructuredVolumeUChar_getVoxel_32(void *uniform _volume, 
                                                    const varying vec3i &index, 
                                                    varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  // Cast to the actual voxel type.
  const uint8 *uniform voxelData = (const uint8 *uniform) volume->voxelData;

  // The voxel value at the given index.
  value = voxelData[index.x + volume->super.dimensions.x * (index.y + volume->super.dimensions.y * index.z)];
}


// -----------------------------------------------------------------------------
// versions for 64/32-bit addressing. volume itself can be larger than
// 2G, but each slice must be within the 2G limit.
// -----------------------------------------------------------------------------


/*! get a voxel from given volume type, assuming we need proper 64-bit addressing */
inline void SharedStructuredVolumeFloat_getVoxel_64_32(void *uniform _volume, 
                                                       const varying vec3i &index, 
                                                       varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  const uniform uint8 *uniform basePtr = (const uniform uint8 *uniform)volume->voxelData;

  // iterate over slices, then do 32-bit gather in slice
  const uint32 ofs = index.x + volume->super.dimensions.x * index.y;
  foreach_unique (z in index.z) {
    const uniform uint64 byteOffset = z * volume->bytesPerSlice;
    const uniform float *uniform sliceData
      = (const uniform float *uniform )(basePtr + byteOffset);
    value = sliceData[ofs];
  }
}

/*! get a voxel from given volume type, assuming we need proper 64-bit addressing */
inline void SharedStructuredVolumeUChar_getVoxel_64_32(void *uniform _volume, 
                                                       const varying vec3i &index, 
                                                       varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;


  const uniform uint8 *uniform basePtr = (const uniform uint8 *uniform)volume->voxelData;

  // iterate over slices, then do 32-bit gather in slice
  const uint32 ofs = index.x + volume->super.dimensions.x * index.y;
  foreach_unique (z in index.z) {
    const uniform uint64 byteOffset = z * volume->bytesPerSlice;
    const uniform uint8 *uniform sliceData
      = (const uniform uint8 *uniform )(basePtr + byteOffset);
    value = sliceData[ofs];
  }
}


// -----------------------------------------------------------------------------
// versions for full 64-bit addressing, no matter what the dimensions or slice size
// -----------------------------------------------------------------------------

/*! get a voxel from given volume type, assuming we need proper 64-bit addressing */
inline void SharedStructuredVolumeFloat_getVoxel_64(void *uniform _volume, 
                                                       const varying vec3i &index, 
                                                       varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  const uint64 index64 = (uint64)index.x + volume->super.dimensions.x * ((int64)index.y + volume->super.dimensions.y * ((uint64)index.z));
  const uint32 hi28 = index64 >> 28;
  const uint32 lo28 = index64 & ((1<<28)-1);

  foreach_unique (hi in hi28) {
    const uniform uint64 hi64 = hi;
    const float *uniform base = ((const float *)volume->voxelData) + (hi64<<28);
    value = base[lo28];
}
}

/*! get a voxel from given volume type, assuming we need proper 64-bit addressing */
inline void SharedStructuredVolumeUChar_getVoxel_64(void *uniform _volume, 
                                                       const varying vec3i &index, 
                                                       varying float &value)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  const uint64 index64 = 
    (uint64)index.x + volume->super.dimensions.x * ((int64)index.y + volume->super.dimensions.y * ((uint64)index.z));
  const uint32 hi28 = index64 >> 28;
  const uint32 lo28 = index64 & ((1<<28)-1);

  foreach_unique (hi in hi28) {
    const uniform uint64 hi64 = hi;
    const uint8 *uniform base = ((const uint8 *)volume->voxelData) + (hi64<<28);
    value = base[lo28];
  }
}


void SharedStructuredVolume_Constructor(SharedStructuredVolume *uniform volume, 
                                        void *uniform cppEquivalent,
                                        const uniform int voxelType, 
                                        const uniform vec3i &dimensions, 
                                        const void *uniform voxelData)
{
  StructuredVolume_Constructor(&volume->super, cppEquivalent, dimensions);

  const uniform uint64 bytesPerVoxel = (voxelType == OSP_FLOAT) ? sizeof(uniform float) : sizeof(uniform uint8);
  const uniform uint64 bytesPerSlice = bytesPerVoxel * (uint64)dimensions.x * (uint64)dimensions.y;
  const uniform uint64 bytesPerVolume = bytesPerSlice * dimensions.z;

  volume->voxelType     = (OSPDataType) voxelType;
  volume->voxelData     = voxelData;
  volume->bytesPerSlice = bytesPerSlice;

  if (bytesPerVolume <= (1ULL<<30)) {
    //print("#osp:shared_structured_volume: using 32-bit mode\n");
    // in this case, we know ALL addressing can be 32-bit.
    volume->super.getVoxel
      = (volume->voxelType == OSP_FLOAT) 
      ? SharedStructuredVolumeFloat_getVoxel_32
      : SharedStructuredVolumeUChar_getVoxel_32;
  } else if (bytesPerSlice <= (1ULL << 30)) {
    //print("#osp:shared_structured_volume: using 64/32-bit mode\n");
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice begins
    volume->super.getVoxel
      = (volume->voxelType == OSP_FLOAT) 
      ? SharedStructuredVolumeFloat_getVoxel_64_32
      : SharedStructuredVolumeUChar_getVoxel_64_32;
  } else {
    //print("#osp:shared_structured_volume: using 64-bit mode\n");
    // in this case, even a single slice is too big to do 32-bit
    // addressing, and we have to do 64-bit throughout
    volume->super.getVoxel
      = (volume->voxelType == OSP_FLOAT) 
      ? SharedStructuredVolumeFloat_getVoxel_64
      : SharedStructuredVolumeUChar_getVoxel_64;
  }
}

export void *uniform SharedStructuredVolume_createInstance(void *uniform cppEquivalent, 
                                                           const uniform int voxelType, 
                                                           const uniform vec3i &dimensions, 
                                                           const void *uniform voxelData)
{
  // The volume container.
  SharedStructuredVolume *uniform volume = uniform new uniform SharedStructuredVolume;

  SharedStructuredVolume_Constructor(volume, cppEquivalent, voxelType, dimensions, voxelData);

  return volume;
}
