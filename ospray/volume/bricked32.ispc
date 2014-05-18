#include "volume.ih"
#include "bricked32.ih"

#define BRICK_SIZE (1<<BRICK_BITS)
#define BRICK_MASK (BRICK_SIZE-1)

// ------------------------------------------------------------------
// the data structure itself
// ------------------------------------------------------------------

/*! generic bricked volume class */
struct BrickedVolume {
  Volume inherited;
  vec3i  bricks;
};


// ------------------------------------------------------------------
// 'internal' functions
// ------------------------------------------------------------------

inline int BrickedVolume_getCoord(uniform BrickedVolume *uniform volume,
                                  const vec3i &pos)
{
  const int brick_x = pos.x >> BRICK_BITS;
  const int brick_y = pos.y >> BRICK_BITS;
  const int brick_z = pos.z >> BRICK_BITS;
  
  const int ofs_x = pos.x & ((1<<BRICK_BITS)-1);
  const int ofs_y = pos.y & ((1<<BRICK_BITS)-1);
  const int ofs_z = pos.z & ((1<<BRICK_BITS)-1);
  
  const int brickID
    = brick_x + volume->bricks.x * (brick_y + volume->bricks.y * brick_z);
  int coord = brickID;
  coord = (coord << BRICK_BITS) | ofs_z;
  coord = (coord << BRICK_BITS) | ofs_y;
  coord = (coord << BRICK_BITS) | ofs_x;
  return coord;
}


/*! set given region to voxel data specified in 'data'. The region
    *must* be inside the volume, and the 'data' type must match the
    internal data format */
void Bricked32Volume1uc_setRegion(uniform VolumePtr _volume,             
                                  const uniform vec3i &begin,                 
                                  const uniform vec3i &size,                  
                                  const void *uniform data)
{
  uniform BrickedVolume *uniform volume 
    = (uniform BrickedVolume *uniform)_volume;

  unsigned int8 *uniform dst = (unsigned int8 *uniform)volume->inherited.data;
  unsigned int8 *uniform src = (unsigned int8 *uniform)data;
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int src_idx = sx + size.x * (sy + size.y * sz);

      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const vec3i pos = make_vec3i(dx,dy,dz);
      const int dst_idx = BrickedVolume_getCoord(volume,pos);
      dst[dst_idx] = src[src_idx];
    }
}
void Bricked32Volume1f_setRegion(uniform VolumePtr _volume,             
                                  const uniform vec3i &begin,                 
                                  const uniform vec3i &size,                  
                                  const void *uniform data)
{
  uniform BrickedVolume *uniform volume 
    = (uniform BrickedVolume *uniform)_volume;

  float *uniform dst = (float *uniform)volume->inherited.data;
  float *uniform src = (float *uniform)data;
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int src_idx = sx + size.x * (sy + size.y * sz);

      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const vec3i pos = make_vec3i(dx,dy,dz);
      const int dst_idx = BrickedVolume_getCoord(volume,pos);
      dst[dst_idx] = src[src_idx];
    }
}




/*! get voxel for bricked volume with 1uc data format and 32-bit addressing */
inline float getVoxel1uc32(uniform BrickedVolume *uniform volume,
                           const vec3i &coord)
{
  const uniform uint8 *uniform data
    = (const uniform uint8 *uniform )volume->inherited.data;
  const int idx = BrickedVolume_getCoord(volume,coord);
  return data[idx];
}

// float gatherFloat(const uniform float *uniform data, const int idx)
// {
//   // print("BEFORE");
//   const float f = data[idx];;
//   print("ab");
//   return f;
// }

/*! get voxel for bricked volume with 1uc data format and 32-bit addressing */
inline 
float getVoxel1f32(uniform BrickedVolume *uniform volume,
                   const vec3i &coord)
{
  const uniform float *uniform data
    = (const uniform float *uniform )volume->inherited.data;
  const int idx = BrickedVolume_getCoord(volume,coord);
  return data[idx];
}

typedef uniform uint32 *varying uniUint32VaryPtr;

inline float getInt8AsInt32(const uniform uint8 *uniform data, const varying uint32 idx)
{
  const int shift = 8*(idx&3);
  const uniform uint8 *varying ptr = (data)+(idx & ~3);
  uint32 asInt32 = (*((uniUint32VaryPtr)(ptr)) >> shift) & 255;
  return asInt32;
}

#if 1
// -------------------------------------------------------
// with 8 absolute addresses
// -------------------------------------------------------
varying float Bricked32Volume1uc_lerpf(uniform VolumePtr _volume,            
                                       const varying vec3f &pos)
{
  uniform BrickedVolume *uniform volume = (uniform BrickedVolume *uniform)_volume;
  const float clamped_x = clamp(pos.x,0.f,volume->inherited.clampSize.x);
  const float clamped_y = clamp(pos.y,0.f,volume->inherited.clampSize.y);
  const float clamped_z = clamp(pos.z,0.f,volume->inherited.clampSize.z);

  const int32 x0 = (int)(clamped_x);
  const int32 y0 = (int)(clamped_y);
  const int32 z0 = (int)(clamped_z);

  const float fx = clamped_x - (float)x0;
  const float fy = clamped_y - (float)y0;
  const float fz = clamped_z - (float)z0;

  const uint32 x1 = x0 + 1;
  const uint32 y1 = y0 + 1;
  const uint32 z1 = z0 + 1;

  const int scale_x = (1<<(3*BRICK_BITS));
  const int scale_y = scale_x * volume->bricks.x;
  const int scale_z = scale_y * volume->bricks.y;

  const int brickOfs_x0 = (x0 >> BRICK_BITS)*scale_x;
  const int brickOfs_y0 = (y0 >> BRICK_BITS)*scale_y;
  const int brickOfs_z0 = (z0 >> BRICK_BITS)*scale_z;
  const int brickOfs_x1 = (x1 >> BRICK_BITS)*scale_x;
  const int brickOfs_y1 = (y1 >> BRICK_BITS)*scale_y;
  const int brickOfs_z1 = (z1 >> BRICK_BITS)*scale_z;

  const int ofs_x0 = brickOfs_x0 + ((x0 & ((1<<BRICK_BITS)-1)));
  const int ofs_y0 = brickOfs_y0 + ((y0 & ((1<<BRICK_BITS)-1)) << BRICK_BITS);
  const int ofs_z0 = brickOfs_z0 + ((z0 & ((1<<BRICK_BITS)-1)) << (BRICK_BITS+BRICK_BITS));
  const int ofs_x1 = brickOfs_x1 + ((x1 & ((1<<BRICK_BITS)-1)));
  const int ofs_y1 = brickOfs_y1 + ((y1 & ((1<<BRICK_BITS)-1)) << BRICK_BITS);
  const int ofs_z1 = brickOfs_z1 + ((z1 & ((1<<BRICK_BITS)-1)) << (BRICK_BITS+BRICK_BITS));

  const uniform uint8 *uniform data
    = (const uniform uint8 *uniform )volume->inherited.data;
#if 0
  const float v000 = getInt8AsInt32(data,ofs_x0 + ofs_y0 + ofs_z0);
  const float v001 = getInt8AsInt32(data,ofs_x1 + ofs_y0 + ofs_z0);
  const float v010 = getInt8AsInt32(data,ofs_x0 + ofs_y1 + ofs_z0);
  const float v011 = getInt8AsInt32(data,ofs_x1 + ofs_y1 + ofs_z0);
  const float v100 = getInt8AsInt32(data,ofs_x0 + ofs_y0 + ofs_z1);
  const float v101 = getInt8AsInt32(data,ofs_x1 + ofs_y0 + ofs_z1);
  const float v110 = getInt8AsInt32(data,ofs_x0 + ofs_y1 + ofs_z1);
  const float v111 = getInt8AsInt32(data,ofs_x1 + ofs_y1 + ofs_z1);
#else
  const float v000 = getVoxel1uc32(volume,make_vec3i(x0,y0,z0));
  const float v001 = getVoxel1uc32(volume,make_vec3i(x1,y0,z0));
  const float v010 = getVoxel1uc32(volume,make_vec3i(x0,y1,z0));
  const float v011 = getVoxel1uc32(volume,make_vec3i(x1,y1,z0));
  const float v100 = getVoxel1uc32(volume,make_vec3i(x0,y0,z1));
  const float v101 = getVoxel1uc32(volume,make_vec3i(x1,y0,z1));
  const float v110 = getVoxel1uc32(volume,make_vec3i(x0,y1,z1));
  const float v111 = getVoxel1uc32(volume,make_vec3i(x1,y1,z1));
#endif
  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  return v * (1.f/255.f);
}
#else
// -------------------------------------------------------
// with delta x, delta y, delta z
// -------------------------------------------------------
varying float Bricked32Volume1uc_lerpf(uniform VolumePtr _volume,            
                                       const varying vec3f &pos)
{
  uniform BrickedVolume *uniform volume = (uniform BrickedVolume *uniform)_volume;
  const float clamped_x = clamp(pos.x,0.f,volume->inherited.clampSize.x);
  const float clamped_y = clamp(pos.y,0.f,volume->inherited.clampSize.y);
  const float clamped_z = clamp(pos.z,0.f,volume->inherited.clampSize.z);

  const int32 x0 = (int)(clamped_x);
  const int32 y0 = (int)(clamped_y);
  const int32 z0 = (int)(clamped_z);

  const float fx = clamped_x - (float)x0;
  const float fy = clamped_y - (float)y0;
  const float fz = clamped_z - (float)z0;

  const uint32 ofs_dx
    = ((x0 & BRICK_MASK)==BRICK_MASK)
    ? (BRICK_SIZE*BRICK_SIZE*BRICK_SIZE)-BRICK_MASK
    : 1; 
  const uint32 ofs_dy
    = ((y0 & BRICK_MASK)==BRICK_MASK)
    ? volume->bricks.x*(BRICK_SIZE*BRICK_SIZE*BRICK_SIZE)-BRICK_MASK*BRICK_SIZE
    : BRICK_SIZE; 
  const uint32 ofs_dz
    = ((z0 & BRICK_MASK)==BRICK_MASK)
    ? volume->bricks.x*volume->bricks.y*(BRICK_SIZE*BRICK_SIZE*BRICK_SIZE)-BRICK_MASK*BRICK_SIZE*BRICK_SIZE
    : BRICK_SIZE*BRICK_SIZE; 
   
  const uint32 y1 = y0 + 1;
  const uint32 z1 = z0 + 1;

  const int scale_x = (1<<(3*BRICK_BITS));
  const int scale_y = scale_x * volume->bricks.x;
  const int scale_z = scale_y * volume->bricks.y;

  const int brickOfs_x0 = (x0 >> BRICK_BITS)*scale_x;
  const int brickOfs_y0 = (y0 >> BRICK_BITS)*scale_y;
  const int brickOfs_z0 = (z0 >> BRICK_BITS)*scale_z;

  const int ofs_x0 = brickOfs_x0 + ((x0 & ((1<<BRICK_BITS)-1)));
  const int ofs_y0 = brickOfs_y0 + ((y0 & ((1<<BRICK_BITS)-1)) << BRICK_BITS);
  const int ofs_z0 = brickOfs_z0 + ((z0 & ((1<<BRICK_BITS)-1)) << (BRICK_BITS+BRICK_BITS));
  const int ofs = ofs_x0+ofs_y0+ofs_z0;

  const uniform uint8 *uniform data
    = (const uniform uint8 *uniform )volume->inherited.data;

  const float v000 = getInt8AsInt32(data,ofs);
  const float v001 = getInt8AsInt32(data,ofs+ofs_dx);
  const float v010 = getInt8AsInt32(data,ofs+ofs_dy);
  const float v011 = getInt8AsInt32(data,ofs+ofs_dx+ofs_dy);
  const float v100 = getInt8AsInt32(data,ofs+ofs_dz);
  const float v101 = getInt8AsInt32(data,ofs+ofs_dx+ofs_dz);
  const float v110 = getInt8AsInt32(data,ofs+ofs_dy+ofs_dz);
  const float v111 = getInt8AsInt32(data,ofs+ofs_dx+ofs_dy+ofs_dz);

  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  return v * (1.f/255.f);
}
#endif


varying float Bricked32Volume1f_lerpf(uniform VolumePtr _volume,            
                                       const varying vec3f &pos)
{
  uniform BrickedVolume *uniform volume = (uniform BrickedVolume *uniform)_volume;
  const float clamped_x = clamp(pos.x,0.f,volume->inherited.clampSize.x);
  const float clamped_y = clamp(pos.y,0.f,volume->inherited.clampSize.y);
  const float clamped_z = clamp(pos.z,0.f,volume->inherited.clampSize.z);

  const int32 x0 = (int)(clamped_x);
  const int32 y0 = (int)(clamped_y);
  const int32 z0 = (int)(clamped_z);

  const float fx = clamped_x - (float)x0;
  const float fy = clamped_y - (float)y0;
  const float fz = clamped_z - (float)z0;

  const uint32 x1 = x0 + 1;
  const uint32 y1 = y0 + 1;
  const uint32 z1 = z0 + 1;

#if 1
  const int scale_x = (1<<(3*BRICK_BITS));
  const int scale_y = scale_x * volume->bricks.x;
  const int scale_z = scale_y * volume->bricks.y;

  const int brickOfs_x0 = (x0 >> BRICK_BITS)*scale_x;
  const int brickOfs_y0 = (y0 >> BRICK_BITS)*scale_y;
  const int brickOfs_z0 = (z0 >> BRICK_BITS)*scale_z;
  const int brickOfs_x1 = (x1 >> BRICK_BITS)*scale_x;
  const int brickOfs_y1 = (y1 >> BRICK_BITS)*scale_y;
  const int brickOfs_z1 = (z1 >> BRICK_BITS)*scale_z;

  // iw: could actually do this a _bit_ faster with blends - we
  // essentially blend the respect lower bits into the upper bits from
  // the bricksofs, but that reqiures bit-blends which AVX supports,
  // but which ISPC doesn't.
  const int ofs_x0 = brickOfs_x0 + ((x0 & ((1<<BRICK_BITS)-1)));
  const int ofs_y0 = brickOfs_y0 + ((y0 & ((1<<BRICK_BITS)-1)) << BRICK_BITS);
  const int ofs_z0 = brickOfs_z0 + ((z0 & ((1<<BRICK_BITS)-1)) << (BRICK_BITS+BRICK_BITS));
  const int ofs_x1 = brickOfs_x1 + ((x1 & ((1<<BRICK_BITS)-1)));
  const int ofs_y1 = brickOfs_y1 + ((y1 & ((1<<BRICK_BITS)-1)) << BRICK_BITS);
  const int ofs_z1 = brickOfs_z1 + ((z1 & ((1<<BRICK_BITS)-1)) << (BRICK_BITS+BRICK_BITS));
  
  const uniform float *uniform data
    = (const uniform float *uniform )volume->inherited.data;
  const float v000 = data[ofs_x0 + ofs_y0 + ofs_z0];
  const float v001 = data[ofs_x1 + ofs_y0 + ofs_z0];
  const float v010 = data[ofs_x0 + ofs_y1 + ofs_z0];
  const float v011 = data[ofs_x1 + ofs_y1 + ofs_z0];
  const float v100 = data[ofs_x0 + ofs_y0 + ofs_z1];
  const float v101 = data[ofs_x1 + ofs_y0 + ofs_z1];
  const float v110 = data[ofs_x0 + ofs_y1 + ofs_z1];
  const float v111 = data[ofs_x1 + ofs_y1 + ofs_z1];

#else

  // print("blubb % % %\n",x0,y0,z0);  
  const float v000 = getVoxel1f32(volume,make_vec3i(x0,y0,z0));
  const float v001 = getVoxel1f32(volume,make_vec3i(x1,y0,z0));
  const float v010 = getVoxel1f32(volume,make_vec3i(x0,y1,z0));
  const float v011 = getVoxel1f32(volume,make_vec3i(x1,y1,z0));
  const float v100 = getVoxel1f32(volume,make_vec3i(x0,y0,z1));
  const float v101 = getVoxel1f32(volume,make_vec3i(x1,y0,z1));
  const float v110 = getVoxel1f32(volume,make_vec3i(x0,y1,z1));
  const float v111 = getVoxel1f32(volume,make_vec3i(x1,y1,z1));
#endif
  
  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);
  // print("DONE blubb % % %\n",x1,y1,z1);  

  return v;
}

varying float Bricked32Volume1uc_gradf(uniform VolumePtr volume,            
                                    varying vec3f &grad,            
                                    const varying vec3f &pos)
{
  grad.x
    = Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
    - Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
  grad.y
    = Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
    - Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
  grad.z
    = Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
    - Bricked32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
  return 
    Bricked32Volume1uc_lerpf(volume,pos);
}
varying float Bricked32Volume1f_gradf(uniform VolumePtr volume,            
                                    varying vec3f &grad,            
                                    const varying vec3f &pos)
{
  grad.x
    = Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
    - Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
  grad.y
    = Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
    - Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
  grad.z
    = Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
    - Bricked32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
  return 
    Bricked32Volume1f_lerpf(volume,pos);
}

inline uniform int divRoundUp(uniform int nom, uniform int den)
{
  return (nom + den - 1)/den;
}


uniform VolumePtr Bricked32Volume1f_create(uniform vec3i &size, const float *uniform internal)
{
  // print("creating bricked volume, size % % %\n",size.x,size.y,size.z);
  uniform BrickedVolume *uniform volume = uniform new uniform BrickedVolume;
  const uniform int brickRes = 1<<BRICK_BITS;
  volume->bricks.x = divRoundUp(size.x,brickRes);
  volume->bricks.y = divRoundUp(size.y,brickRes);
  volume->bricks.z = divRoundUp(size.z,brickRes);
  // print("num bricks % % %\n",volume->bricks.x,volume->bricks.y,volume->bricks.z);
  const uniform int64 numBricks
    = ((int64)volume->bricks.x) * volume->bricks.y * volume->bricks.z;
  const uniform int brickSize = brickRes*brickRes*brickRes;
  const uniform int numElements = numBricks * brickSize;
  volume->inherited.size     = size;
  volume->inherited.dataSize = numElements * sizeof(uniform float);
  if (internal)
    volume->inherited.data = (void *uniform) internal;
  else
  volume->inherited.data     = uniform new uniform float[numElements];
  
  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  
  // volume->inherited.setRegion = Bricked32Volume1f_setRegion;
  volume->inherited.lerpf     = Bricked32Volume1f_lerpf;
  volume->inherited.gradf     = Bricked32Volume1f_gradf;
  return (uniform VolumePtr)volume;
}

uniform VolumePtr Bricked32Volume1uc_create(uniform vec3i &size, const uint8 *uniform internal)
{
  // print("creating bricked volume, size % % %\n",size.x,size.y,size.z);
  uniform BrickedVolume *uniform volume = uniform new uniform BrickedVolume;
  const uniform int brickRes = 1<<BRICK_BITS;
  volume->bricks.x = divRoundUp(size.x,brickRes);
  volume->bricks.y = divRoundUp(size.y,brickRes);
  volume->bricks.z = divRoundUp(size.z,brickRes);
  // print("num bricks % % %\n",volume->bricks.x,volume->bricks.y,volume->bricks.z);
  const uniform int64 numBricks
    = ((int64)volume->bricks.x) * volume->bricks.y * volume->bricks.z;
  const uniform int brickSize = brickRes*brickRes*brickRes;
  const uniform int numElements = numBricks * brickSize;
  volume->inherited.size     = size;
  volume->inherited.dataSize = numElements * sizeof(uniform int8);
  if (internal)
    volume->inherited.data = (void *uniform) internal;
  else
    volume->inherited.data     = uniform new uniform int8[numElements];

  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  
  // volume->inherited.setRegion = Bricked32Volume1uc_setRegion;
  volume->inherited.lerpf     = Bricked32Volume1uc_lerpf;
  volume->inherited.gradf     = Bricked32Volume1uc_gradf;
  return (uniform VolumePtr)volume;
}

// ------------------------------------------------------------------
// publicly exported functions at the end
// ------------------------------------------------------------------

export void _Bricked32Volume1uc_setRegion(uniform ExtVolumePtr _volume,
                                         const uniform vec3i &begin,
                                         const uniform vec3i &size,
                                         const uniform unsigned int8 *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  Bricked32Volume1uc_setRegion(volume,begin,size,data);
}

export void _Bricked32Volume1f_setRegion(uniform ExtVolumePtr _volume,
                                         const uniform vec3i &begin,
                                         const uniform vec3i &size,
                                         const uniform float *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  Bricked32Volume1f_setRegion(volume,begin,size,data);
}


/* need this workaround because ISPC cannot export pointer to 'Volume'
   type because volume type has function pointers */
export uniform ExtVolumePtr _Bricked32Volume1uc_create(uniform vec3i &size, const uint8 *uniform internal)
{ return (ExtVolumePtr)Bricked32Volume1uc_create(size,internal); }
/* need this workaround because ISPC cannot export pointer to 'Volume'
   type because volume type has function pointers */
export uniform ExtVolumePtr _Bricked32Volume1f_create(uniform vec3i &size, const float *uniform internal)
{ return (ExtVolumePtr)Bricked32Volume1f_create(size,internal); }

