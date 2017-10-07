// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "SharedStructuredVolume.ih"

/*! get a voxel from given volume type */

#define template_getVoxel(type)                                              \
/* --------------------------------------------------------------------------\
// versions for pure 32-bit addressing. volume *MUST* be smaller than 2G     \
// ------------------------------------------------------------------------*/\
inline void SSV_getVoxel_##type##_32(void *uniform _self,                    \
                                     const varying vec3i &index,             \
                                     varying float &value)                   \
{                                                                            \
  /* Cast to the actual Volume subtype. */                                   \
  SharedStructuredVolume *uniform self                                       \
      = (SharedStructuredVolume *uniform)_self;                              \
                                                                             \
  /* Cast to the actual voxel type. */                                       \
  const type *uniform voxelData = (const type *uniform)self->voxelData;      \
  const uint32 addr = index.x +                                              \
      self->super.dimensions.x*(index.y + self->super.dimensions.y*index.z); \
                                                                             \
  /* The voxel value at the given index. */                                  \
  value = voxelData[addr];                                                   \
}                                                                            \
/* --------------------------------------------------------------------------\
// versions for 64/32-bit addressing. volume itself can be larger than       \
// 2G, but each slice must be within the 2G limit.                           \
// ------------------------------------------------------------------------*/\
inline void SSV_getVoxel_##type##_64_32(void *uniform _self,                 \
                                        const varying vec3i &index,          \
                                        varying float &value)                \
{                                                                            \
  /* Cast to the actual Volume subtype. */                                   \
  SharedStructuredVolume *uniform self                                       \
      = (SharedStructuredVolume *uniform)_self;                              \
                                                                             \
  const uniform uint8 *uniform basePtr =                                     \
      (const uniform uint8 *uniform)self->voxelData;                         \
                                                                             \
  /* iterate over slices, then do 32-bit gather in slice */                  \
  const uint32 ofs = index.x + self->super.dimensions.x * index.y;           \
  foreach_unique (z in index.z) {                                            \
    const uniform uint64 byteOffset = z * self->bytesPerSlice;               \
    const uniform type *uniform sliceData                                    \
      = (const uniform type *uniform )(basePtr + byteOffset);                \
    value = sliceData[ofs];                                                  \
  }                                                                          \
}                                                                            \
/* --------------------------------------------------------------------------\
// versions for full 64-bit addressing, for all dimensions or slice size     \
// ------------------------------------------------------------------------*/\
inline void SSV_getVoxel_##type##_64(void *uniform _self,                    \
                                     const varying vec3i &index,             \
                                     varying float &value)                   \
{                                                                            \
  /* Cast to the actual Volume subtype. */                                   \
  SharedStructuredVolume *uniform self                                       \
      = (SharedStructuredVolume *uniform)_self;                              \
                                                                             \
  const uint64 index64 = (uint64)index.x + self->super.dimensions.x *        \
      ((int64)index.y + self->super.dimensions.y * ((uint64)index.z));       \
  const uint32 hi28 = index64 >> 28;                                         \
  const uint32 lo28 = index64 & ((1<<28)-1);                                 \
                                                                             \
  foreach_unique (hi in hi28) {                                              \
    const uniform uint64 hi64 = hi;                                          \
    const type *uniform base = ((const type *)self->voxelData) + (hi64<<28); \
    value = base[lo28];                                                      \
  }                                                                          \
}

template_getVoxel(uint8);
template_getVoxel(int16);
template_getVoxel(uint16);
template_getVoxel(float);
template_getVoxel(double);
#undef template_getVoxel


/*! read a _typed_ value from an address that's given by an
    *BYTE*-offset relative to a base array. note that even though we
    assume that the offset is already in bytes (ie, WITHOUT scaling by
    the width of the array data type), the the base pointer must
    *STILL* be of the proper type for this macro to figure out the
    type of data it's reading from this address */
#define template_accessArray(type)                                           \
inline float accessArrayWithOffset(const type *uniform basePtr,              \
                                   const varying uint32 offset)              \
{                                                                            \
  uniform uint8 *uniform base = (uniform uint8 *uniform)basePtr;             \
  return *((uniform type *)(base+offset));                                   \
}                                                                            \
inline float accessArrayWithOffset(const type *uniform basePtr,              \
                                   const uniform uint64 baseOfs,             \
                                   const varying uint32 offset)              \
{                                                                            \
  uniform uint8 *uniform base = (uniform uint8 *uniform)(basePtr);           \
  return *((uniform type *)((base+baseOfs)+offset));                         \
}                                                                            \

template_accessArray(uint8);
template_accessArray(int16);
template_accessArray(uint16);
template_accessArray(float);
template_accessArray(double);
#undef template_accessArray


/*! perform trilinear interpolation for given sample. unlike old way
  of doing this (a single computesample on the StructuredVolume level
  that calls the virtual 'getSample()' of the volume layout) this
  function will directly do all the addressing for the getSample
  (inlined), and thus be about 50% faster (wall-time, meaning even
  much faster in pure sample speed) */
#define template_sample(type)                                         \
inline float SSV_sample_##type##_32(void *uniform _self,              \
                                           const vec3f &worldCoordinates)    \
{                                                                            \
  /* Cast to the actual Volume subtype. */                                   \
  SharedStructuredVolume *uniform self                                       \
      = (SharedStructuredVolume *uniform)_self;                              \
                                                                             \
  /* Transform the sample location into the local coordinate system. */      \
  vec3f localCoordinates;                                                    \
  self->super.transformWorldToLocal(&self->super,                            \
                                    worldCoordinates,                        \
                                    localCoordinates);                       \
                                                                             \
  /* Coordinates outside the volume are clamped to the volume bounds. */     \
  const vec3f clampedLocalCoordinates                                        \
      = clamp(localCoordinates, make_vec3f(0.0f),                            \
              self->super.localCoordinatesUpperBound);                       \
                                                                             \
  /* Lower and upper corners of the box straddling the voxels to be interpolated. */\
  const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);          \
                                                                             \
  /* Fractional coordinates within the lower corner voxel used during interpolation. */\
  const vec3f frac = clampedLocalCoordinates - float_cast(voxelIndex_0);     \
                                                                             \
  const uint32 voxelOfs                                                      \
    = voxelIndex_0.x * self->voxelOfs_dx                                     \
    + voxelIndex_0.y * self->voxelOfs_dy                                     \
    + voxelIndex_0.z * self->voxelOfs_dz;                                    \
  const type *uniform voxelData = (const type *uniform)self->voxelData;      \
  const uniform uint64 ofs000 = 0;                                           \
  const uniform uint64 ofs001 = self->bytesPerVoxel;                         \
  const float val000  = accessArrayWithOffset(voxelData,ofs000,voxelOfs);    \
  const float val001  = accessArrayWithOffset(voxelData,ofs001,voxelOfs);    \
  const float val00   = val000 + frac.x * (val001 - val000);                 \
                                                                             \
  const uniform uint64 ofs010 = self->bytesPerLine;                          \
  const uniform uint64 ofs011 = self->bytesPerLine+self->bytesPerVoxel;      \
  const float val010  = accessArrayWithOffset(voxelData,ofs010,voxelOfs);    \
  const float val011  = accessArrayWithOffset(voxelData,ofs011,voxelOfs);    \
  const float val01   = val010 + frac.x * (val011 - val010);                 \
                                                                             \
  const uniform uint64 ofs100 = self->bytesPerSlice;                         \
  const uniform uint64 ofs101 = ofs100 + ofs001;                             \
  const float val100  = accessArrayWithOffset(voxelData,ofs100,voxelOfs);    \
  const float val101  = accessArrayWithOffset(voxelData,ofs101,voxelOfs);    \
  const float val10   = val100 + frac.x * (val101 - val100);                 \
                                                                             \
  const uniform uint64 ofs110 = ofs100 + ofs010;                             \
  const uniform uint64 ofs111 = ofs100 + ofs011;                             \
  const float val110  = accessArrayWithOffset(voxelData,ofs110,voxelOfs);    \
  const float val111  = accessArrayWithOffset(voxelData,ofs111,voxelOfs);    \
  const float val11   = val110 + frac.x * (val111 - val110);                 \
                                                                             \
  /* Interpolate the voxel values. */                                        \
  const float val0    = val00  + frac.y * (val01  - val00);                  \
  const float val1    = val10  + frac.y * (val11  - val10);                  \
  const float val     = val0   + frac.z * (val1   - val0 );                  \
                                                                             \
  return val;                                                                \
}                                                                            \
                                                                             \
inline float SSV_sample_##type##_64_32(void *uniform _self,           \
                                              const vec3f &worldCoordinates) \
{                                                                            \
  /* Cast to the actual Volume subtype. */                                   \
  SharedStructuredVolume *uniform self                                       \
      = (SharedStructuredVolume *uniform)_self;                              \
                                                                             \
  /* Transform the sample location into the local coordinate system. */      \
  vec3f localCoordinates;                                                    \
  self->super.transformWorldToLocal(&self->super,                            \
                                    worldCoordinates,                        \
                                    localCoordinates);                       \
                                                                             \
  /* Coordinates outside the volume are clamped to the volume bounds. */     \
  const vec3f clampedLocalCoordinates                                        \
    = clamp(localCoordinates, make_vec3f(0.0f),                              \
            self->super.localCoordinatesUpperBound);                         \
                                                                             \
  /* Lower and upper corners of the box straddling the voxels to be interpolated. */\
  const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);          \
                                                                             \
  /* Fractional coordinates within the lower corner voxel used during interpolation. */\
  const vec3f frac = clampedLocalCoordinates - float_cast(voxelIndex_0);     \
                                                                             \
  varying float ret = 0.f;                                                   \
  foreach_unique (sliceID in voxelIndex_0.z)  {                              \
    const uint32 voxelOfs                                                    \
      = voxelIndex_0.x * self->voxelOfs_dx                                   \
      + voxelIndex_0.y * self->voxelOfs_dy;                                  \
    const type *uniform voxelData                                            \
      = (const type *uniform)((uniform uint8*uniform)self->voxelData         \
                              +sliceID*self->bytesPerSlice);                 \
    const uniform uint64 ofs000 = 0;                                         \
    const uniform uint64 ofs001 = self->bytesPerVoxel;                       \
    const float val000  = accessArrayWithOffset(voxelData,ofs000,voxelOfs);  \
    const float val001  = accessArrayWithOffset(voxelData,ofs001,voxelOfs);  \
    const float val00   = val000 + frac.x * (val001 - val000);               \
                                                                             \
    const uniform uint64 ofs010 = self->bytesPerLine;                        \
    const uniform uint64 ofs011 = self->bytesPerLine+self->bytesPerVoxel;    \
    const float val010  = accessArrayWithOffset(voxelData,ofs010,voxelOfs);  \
    const float val011  = accessArrayWithOffset(voxelData,ofs011,voxelOfs);  \
    const float val01   = val010 + frac.x * (val011 - val010);               \
                                                                             \
    const uniform uint64 ofs100 = self->bytesPerSlice;                       \
    const uniform uint64 ofs101 = ofs100 + ofs001;                           \
    const float val100  = accessArrayWithOffset(voxelData,ofs100,voxelOfs);  \
    const float val101  = accessArrayWithOffset(voxelData,ofs101,voxelOfs);  \
    const float val10   = val100 + frac.x * (val101 - val100);               \
                                                                             \
    const uniform uint64 ofs110 = ofs100 + ofs010;                           \
    const uniform uint64 ofs111 = ofs100 + ofs011;                           \
    const float val110  = accessArrayWithOffset(voxelData,ofs110,voxelOfs);  \
    const float val111  = accessArrayWithOffset(voxelData,ofs111,voxelOfs);  \
    const float val11   = val110 + frac.x * (val111 - val110);               \
                                                                             \
    /* Interpolate the voxel values. */                                      \
    const float val0    = val00  + frac.y * (val01  - val00);                \
    const float val1    = val10  + frac.y * (val11  - val10);                \
    const float val     = val0   + frac.z * (val1   - val0 );                \
    ret = val;                                                               \
  }                                                                          \
  return ret;                                                                \
}

template_sample(uint8)
template_sample(int16)
template_sample(uint16)
template_sample(float)
template_sample(double)
#undef template_sample


void SharedStructuredVolume_Constructor(SharedStructuredVolume *uniform self,
                                        void *uniform cppEquivalent,
                                        const uniform int voxelType,
                                        const uniform vec3i &dimensions,
                                        const void *uniform voxelData)
{
  StructuredVolume_Constructor(&self->super, cppEquivalent, dimensions);

  uniform uint64 bytesPerVoxel;

  if (voxelType == OSP_UCHAR)
    bytesPerVoxel = sizeof(uniform uint8);
  else if (voxelType == OSP_SHORT)
    bytesPerVoxel = sizeof(uniform int16);
  else if (voxelType == OSP_USHORT)
    bytesPerVoxel = sizeof(uniform uint16);
  else if (voxelType == OSP_FLOAT)
    bytesPerVoxel = sizeof(uniform float);
  else if (voxelType == OSP_DOUBLE)
    bytesPerVoxel = sizeof(uniform double);
  else {
    print("#osp:shared_structured_volume: unknown voxel type\n");
    return;
  }

  const uniform uint64 bytesPerLine = bytesPerVoxel * dimensions.x;
  const uniform uint64 bytesPerSlice = bytesPerLine * dimensions.y;
  const uniform uint64 bytesPerVolume = bytesPerSlice * dimensions.z;

  self->voxelType     = (OSPDataType) voxelType;
  self->voxelData     = voxelData;
  self->bytesPerVoxel = bytesPerVoxel;
  self->bytesPerLine  = bytesPerLine;
  self->bytesPerSlice = bytesPerSlice;
  self->voxelOfs_dx   = bytesPerVoxel;
  self->voxelOfs_dy   = bytesPerLine;
  self->voxelOfs_dz   = bytesPerSlice;

  if (bytesPerVolume <= (1ULL<<30)) {
    //print("#osp:shared_structured_volume: using 32-bit mode\n");
    // in this case, we know ALL addressing can be 32-bit.

    if (voxelType == OSP_UCHAR) {
      self->super.getVoxel = SSV_getVoxel_uint8_32;
      self->super.super.sample = SSV_sample_uint8_32;
    } else if (voxelType == OSP_SHORT) {
      self->super.getVoxel = SSV_getVoxel_int16_32;
      self->super.super.sample = SSV_sample_float_32;
    } else if (voxelType == OSP_USHORT) {
      self->super.getVoxel = SSV_getVoxel_uint16_32;
      self->super.super.sample = SSV_sample_float_32;
    } else if (voxelType == OSP_FLOAT) {
      self->super.getVoxel = SSV_getVoxel_float_32;
      self->super.super.sample = SSV_sample_float_32;
    } else if (voxelType == OSP_DOUBLE) {
      self->super.getVoxel = SSV_getVoxel_double_32;
      self->super.super.sample = SSV_sample_double_32;
    }

  } else if (bytesPerSlice <= (1ULL << 30)) {
    //print("#osp:shared_structured_volume: using 64/32-bit mode\n");
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice begins

    if (voxelType == OSP_UCHAR) {
      self->super.getVoxel = SSV_getVoxel_uint8_64_32;
      self->super.super.sample = SSV_sample_uint8_64_32;
    } else if (voxelType == OSP_SHORT) {
      self->super.getVoxel = SSV_getVoxel_int16_64_32;
      self->super.super.sample = SSV_sample_int16_64_32;
    } else if (voxelType == OSP_USHORT) {
      self->super.getVoxel = SSV_getVoxel_uint16_64_32;
      self->super.super.sample = SSV_sample_uint16_64_32;
    } else if (voxelType == OSP_FLOAT) {
      self->super.getVoxel = SSV_getVoxel_float_64_32;
      self->super.super.sample = SSV_sample_float_64_32;
    } else if (voxelType == OSP_DOUBLE) {
      self->super.getVoxel = SSV_getVoxel_double_64_32;
      self->super.super.sample = SSV_sample_double_64_32;
    }
  } else {
    //print("#osp:shared_structured_volume: using 64-bit mode\n");
    // in this case, even a single slice is too big to do 32-bit
    // addressing, and we have to do 64-bit throughout

    if (voxelType == OSP_UCHAR)
      self->super.getVoxel = SSV_getVoxel_uint8_64;
    else if (voxelType == OSP_SHORT)
      self->super.getVoxel = SSV_getVoxel_int16_64;
    else if (voxelType == OSP_USHORT)
      self->super.getVoxel = SSV_getVoxel_uint16_64;
    else if (voxelType == OSP_FLOAT)
      self->super.getVoxel = SSV_getVoxel_float_64;
    else if (voxelType == OSP_DOUBLE)
      self->super.getVoxel = SSV_getVoxel_double_64;
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

/*! this is a very simple implementation that does not yet use either
  vectors OR threads ... should be fixed */
#define template_setRegion(type)                                             \
export void SharedStructuredVolume_setRegion_##type                          \
(void *uniform _self,                                                        \
 const void *uniform source,                                                 \
 const uniform vec3i &index,                                                 \
 const uniform vec3i &count)                                                 \
{                                                                            \
  SharedStructuredVolume *uniform self                                       \
    = (SharedStructuredVolume *uniform)_self;                                \
  uniform type *uniform ptr_out = (uniform type *)self->voxelData;           \
  const uniform type *uniform ptr_in = (const uniform type *)source;         \
  for (uniform int iz=0;iz<count.z;iz++) {                                   \
    uniform int64 iiz = iz + index.z;                                        \
    for (uniform int iy=0;iy<count.y;iy++)                                   \
      for (uniform int ix=0;ix<count.x;ix++) {                               \
        uniform int64 iix = ix + index.x;                                    \
        uniform int64 iiy = iy + index.y;                                    \
        uniform int64 ofs_in                                                 \
          = ix + count.x*(iy+count.y*(uniform int64)iz);                     \
        uniform int64 ofs_out                                                \
          = iix + self->super.dimensions.x                                   \
          * (iiy+self->super.dimensions.y*iiz);                              \
        *(ptr_out+ofs_out) = ptr_in[ofs_in];                                 \
      }                                                                      \
  }                                                                          \
}

template_setRegion(uint8);
template_setRegion(int16);
template_setRegion(uint16);
template_setRegion(float);
template_setRegion(double);
#undef template_setRegion
