/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "volume.ih"

// ------------------------------------------------------------------
// the data structure itself
// ------------------------------------------------------------------

/*! generic 'naive' volume class (ie, vlume that uses no bricking etc
    whatsoever: plain array order of voxel. */
struct NaiveVolume {
  Volume inherited;
};


// ------------------------------------------------------------------
// 'internal' functions
// ------------------------------------------------------------------

/*! set given region to voxel data specified in 'data'. The region
    *must* be inside the volume, and the 'data' type must match the
    internal data format */
void Naive32Volume1f_setRegion(uniform VolumePtr _volume,             
                                const uniform vec3i &begin,                 
                                const uniform vec3i &size,                  
                                const void *uniform data)
{
  uniform Volume *uniform volume 
    = (uniform Volume *uniform)_volume;
  float *uniform dst = (float *uniform)volume->data;
  float *uniform src = (float *uniform)data;
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const int src_idx = sx + size.x * (sy + size.y * sz);
      const int dst_idx = dx + volume->size.x * (dy + volume->size.y * dz);
      dst[dst_idx] = src[src_idx];
    }
}
/*! set given region to voxel data specified in 'data'. The region
    *must* be inside the volume, and the 'data' type must match the
    internal data format */
void Naive32Volume1uc_setRegion(uniform VolumePtr _volume,             
                                const uniform vec3i &begin,                 
                                const uniform vec3i &size,                  
                                const void *uniform data)
{
  uniform Volume *uniform volume 
    = (uniform Volume *uniform)_volume;
  unsigned int8 *uniform dst = (unsigned int8 *uniform)volume->data;
  unsigned int8 *uniform src = (unsigned int8 *uniform)data;
  // print("SRC % DST %\n",src,dst);
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const int src_idx = sx + size.x * (sy + size.y * sz);
      const int dst_idx = dx + volume->size.x * (dy + volume->size.y * dz);
      // print("% -> %\n",src_idx,dst_idx);
      dst[dst_idx] = src[src_idx];
      // print("DONE % -> %\n",src_idx,dst_idx);
    }
  // print("DONE\n\n");
  // print("DONE\n\n");
  // print("DONE\n\n");
  // print("DONE\n\n");
}

// workaround to cover ISPC issue #606, see below
inline float getVoxel(const uniform unsigned int8 *uniform voxel, 
                     uniform uint32 iOfs, varying uint32 vOfs)
{
  const uniform unsigned int8 *uniform p = voxel + iOfs;
  // print("ofs % % v %\n",iOfs,iOfs+vOfs,p);
  
  return p[vOfs];
}
// workaround to cover ISPC issue #606, see below
inline float getVoxel(const uniform float *uniform voxel, 
                     uniform int iOfs, varying int vOfs)
{
  const uniform float *uniform p = voxel + iOfs;
  const float v = p[vOfs];
  // print("ofs % v %\n",iOfs+vOfs,v);
  return v;
}

/*! compute trilineratly interpolated volume sample in floating point
    accuracy. (sample values outside the volume will be clamped to the
    nearest positoin inside the volume) */
inline varying float Naive32Volume1f_lerpf(uniform VolumePtr _volume,            
                                           const varying vec3f &pos)
{
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  const uniform float *uniform voxel
    = (const uniform float *uniform)volume->data;
  const float clamped_x = clamp(pos.x,0.f,volume->clampSize.x);
  const float clamped_y = clamp(pos.y,0.f,volume->clampSize.y);
  const float clamped_z = clamp(pos.z,0.f,volume->clampSize.z);

  const int x0 = (int)(clamped_x);
  const int y0 = (int)(clamped_y);
  const int z0 = (int)(clamped_z);

  const float fx = clamped_x - (float)x0;
  const float fy = clamped_y - (float)y0;
  const float fz = clamped_z - (float)z0;
  
  const uniform int dx = 1;
  const uniform int dy = volume->size.x;
  const uniform int dz = dy * volume->size.y;


  const int ofs = x0 + volume->size.x * (y0 + volume->size.y * z0);
  // print("size % % %\n",volume->size.x,volume->size.y,volume->size.z);
  // print("xyz % % % -> ofs %\n",x0,y0,z0,ofs);
#if 1
  // workaround to cover ISPC bug #606
  const float v000 = getVoxel(voxel,0,ofs);
  const float v001 = getVoxel(voxel,+dx,ofs);
  const float v010 = getVoxel(voxel,+dy,ofs);
  const float v011 = getVoxel(voxel,+dy+dx,ofs);
  const float v100 = getVoxel(voxel,+dz,ofs);
  const float v101 = getVoxel(voxel,+dz+dx,ofs);
  const float v110 = getVoxel(voxel,+dz+dy,ofs);
  const float v111 = getVoxel(voxel,+dz+dy+dx,ofs);
#else
  const float v000 = (voxel)[ofs];
  const float v001 = (voxel+dx)[ofs];
  const float v010 = (voxel+dy)[ofs];
  const float v011 = (voxel+dy+dx)[ofs];
  const float v100 = (voxel+dz)[ofs];
  const float v101 = (voxel+dz+dx)[ofs];
  const float v110 = (voxel+dz+dy)[ofs];
  const float v111 = (voxel+dz+dy+dx)[ofs];
#endif
  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  // print("v %\n",v);
  return v;
}
/*! compute trilineratly interpolated volume sample in floating point
    accuracy. (sample values outside the volume will be clamped to the
    nearest positoin inside the volume) */
inline varying float Naive32Volume1uc_lerpf(uniform VolumePtr _volume,            
                                    const varying vec3f &pos)
{
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  const uniform unsigned int8 *uniform voxel
    = (const uniform unsigned int8 *uniform)volume->data;
  const float clamped_x = clamp(pos.x,0.f,volume->clampSize.x);
  const float clamped_y = clamp(pos.y,0.f,volume->clampSize.y);
  const float clamped_z = clamp(pos.z,0.f,volume->clampSize.z);

  const int x0 = (int)(clamped_x);
  const int y0 = (int)(clamped_y);
  const int z0 = (int)(clamped_z);

  const float fx = clamped_x - (float)x0;
  const float fy = clamped_y - (float)y0;
  const float fz = clamped_z - (float)z0;

  const uniform int dx = 1;
  const uniform int dy = volume->size.x;
  const uniform int dz = dy * volume->size.y;

  const int ofs = x0 + volume->size.x * (y0 + volume->size.y * z0);
#if 1
  // workaround to cover ISPC bug #606
  const float v000 = getVoxel(voxel,0,ofs);
  const float v001 = getVoxel(voxel,+dx,ofs);
  const float v010 = getVoxel(voxel,+dy,ofs);
  const float v011 = getVoxel(voxel,+dy+dx,ofs);
  const float v100 = getVoxel(voxel,+dz,ofs);
  const float v101 = getVoxel(voxel,+dz+dx,ofs);
  const float v110 = getVoxel(voxel,+dz+dy,ofs);
  const float v111 = getVoxel(voxel,+dz+dy+dx,ofs);
#else
  const float v000 = (voxel)[ofs];
  const float v001 = (voxel+dx)[ofs];
  const float v010 = (voxel+dy)[ofs];
  const float v011 = (voxel+dy+dx)[ofs];
  const float v100 = (voxel+dz)[ofs];
  const float v101 = (voxel+dz+dx)[ofs];
  const float v110 = (voxel+dz+dy)[ofs];
  const float v111 = (voxel+dz+dy+dx)[ofs];
#endif
  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  return v * (1.f/255);
}

inline float lerp(const float fx, const float fy, const float fz,
           const float v111,
           const float v112,
           const float v121,
           const float v122,
           const float v211,
           const float v212,
           const float v221,
           const float v222)
{
  const float v11 = v111 + fx * (v112-v111);
  const float v12 = v121 + fx * (v122-v121);
  const float v21 = v211 + fx * (v212-v211);
  const float v22 = v221 + fx * (v222-v221);

  const float v1 = v11 + fy * (v12-v11);
  const float v2 = v21 + fy * (v22-v21);

  const float v = v1 + fz * (v2-v1);
  return v;
}



/*! compute disrete differences gradient, including clamping */
inline varying float Naive32Volume1uc_gradf(uniform VolumePtr volume,            
                                    varying vec3f &grad,            
                                    const varying vec3f &pos)
{
#if 1
  const uniform unsigned int8 *uniform voxel
    = (const uniform unsigned int8 *uniform)volume->data;
  const float clamped_x1 = clamp(pos.x,0.f,volume->clampSize.x);
  const float clamped_y1 = clamp(pos.y,0.f,volume->clampSize.y);
  const float clamped_z1 = clamp(pos.z,0.f,volume->clampSize.z);
  
  // const float clamped_x0 = max(0.f,clamped_x1-1.f);
  // const float clamped_y0 = max(0.f,clamped_y1-1.f);
  // const float clamped_z0 = max(0.f,clamped_z1-1.f);

  // const float clamped_x2 = min(clamped_x1+1.f,volume->clampSize.x);
  // const float clamped_y2 = min(clamped_y1+1.f,volume->clampSize.y);
  // const float clamped_z2 = min(clamped_z1+1.f,volume->clampSize.z);
  
  const int x1 = (int)clamped_x1;
  const int y1 = (int)clamped_y1;
  const int z1 = (int)clamped_z1;
  
  const float fx = clamped_x1 - (float)x1;
  const float fy = clamped_y1 - (float)y1;
  const float fz = clamped_z1 - (float)z1;

  const uniform int dx = 1;
  const uniform int dy = volume->size.x;
  const uniform int dz = dy * volume->size.y;

  const int ofs = x1 + volume->size.x * (y1 + volume->size.y * z1);
  const float v111 = getVoxel(voxel,0,ofs);
  const float v112 = getVoxel(voxel,+dx,ofs);
  const float v121 = getVoxel(voxel,+dy,ofs);
  const float v122 = getVoxel(voxel,+dy+dx,ofs);
  const float v211 = getVoxel(voxel,+dz,ofs);
  const float v212 = getVoxel(voxel,+dz+dx,ofs);
  const float v221 = getVoxel(voxel,+dz+dy,ofs);
  const float v222 = getVoxel(voxel,+dz+dy+dx,ofs);

  const float v = lerp(fx,fy,fz,v111,v112,v121,v122,v211,v212,v221,v222);
  // const float v11 = v111 + fx * (v112-v111);
  // const float v12 = v121 + fx * (v122-v121);
  // const float v21 = v211 + fx * (v212-v211);
  // const float v22 = v221 + fx * (v222-v221);

  // const float v1 = v11 + fy * (v12-v11);
  // const float v2 = v21 + fy * (v22-v21);

  // const float v = v1 + fz * (v2-v1);

  // -------------------------------------------------------
  const int dx0 = (x1 == 0)?0:dx;
  const int dx3 = ((pos.x+1.f)>volume->clampSize.x)?0:+dx;
  const float v110 = getVoxel(voxel,        0,ofs-dx0);
  const float v113 = getVoxel(voxel,      +dx,ofs+dx3);
  const float v120 = getVoxel(voxel,      +dy,ofs-dx0);
  const float v123 = getVoxel(voxel,   +dy+dx,ofs+dx3);
  const float v210 = getVoxel(voxel,      +dz,ofs-dx0);
  const float v213 = getVoxel(voxel,   +dz+dx,ofs+dx3);
  const float v220 = getVoxel(voxel,   +dz+dy,ofs-dx0);
  const float v223 = getVoxel(voxel,+dz+dy+dx,ofs+dx3);
  grad.x
    = lerp(fx,fy,fz,v112,v113,v122,v123,v212,v213,v222,v223)
    - lerp(fx,fy,fz,v110,v111,v120,v121,v210,v211,v220,v221);

  // -------------------------------------------------------
  const int dy0 = (y1 == 0)?0:dy;
  const int dy3 = ((pos.y+1.f)>volume->clampSize.y)?0:+dy;
  const float v101 = getVoxel(voxel,        0,ofs-dy0);
  const float v102 = getVoxel(voxel,      +dx,ofs-dy0);
  const float v131 = getVoxel(voxel,      +dy,ofs+dy3);
  const float v132 = getVoxel(voxel,   +dy+dx,ofs+dy3);
  const float v201 = getVoxel(voxel,      +dz,ofs-dy0);
  const float v202 = getVoxel(voxel,   +dz+dx,ofs-dy0);
  const float v231 = getVoxel(voxel,   +dz+dy,ofs+dy3);
  const float v232 = getVoxel(voxel,+dz+dy+dx,ofs+dy3);
  grad.y
    = lerp(fx,fy,fz,v121,v122,v131,v132,v221,v222,v231,v232)
    - lerp(fx,fy,fz,v101,v102,v111,v112,v201,v202,v211,v212);

  // -------------------------------------------------------
  const int dz0 = (z1 == 0)?0:dz;
  const int dz3 = ((pos.z+1.f)>volume->clampSize.z)?0:+dz;
  const float v011 = getVoxel(voxel,        0,ofs-dz0);
  const float v012 = getVoxel(voxel,      +dx,ofs-dz0);
  const float v021 = getVoxel(voxel,      +dy,ofs-dz0);
  const float v022 = getVoxel(voxel,   +dy+dx,ofs-dz0);
  const float v311 = getVoxel(voxel,      +dz,ofs+dz3);
  const float v312 = getVoxel(voxel,   +dz+dx,ofs+dz3);
  const float v321 = getVoxel(voxel,   +dz+dy,ofs+dz3);
  const float v322 = getVoxel(voxel,+dz+dy+dx,ofs+dz3);
  grad.z
    = lerp(fx,fy,fz,v211,v212,v221,v222,v311,v312,v321,v322)
    - lerp(fx,fy,fz,v011,v012,v021,v022,v111,v112,v121,v122);

  return v * (1.f/255);
#else
  grad.x
    = Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
    - Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
  grad.y
    = Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
    - Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
  grad.z
    = Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
    - Naive32Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
  return 
    Naive32Volume1uc_lerpf(volume,pos);
#endif
}


/*! compute disrete differences gradient, including clamping */
inline varying float Naive32Volume1f_gradf(uniform VolumePtr volume,            
                                    varying vec3f &grad,            
                                    const varying vec3f &pos)
{
  grad.x
    = Naive32Volume1f_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
    - Naive32Volume1f_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
  grad.y
    = Naive32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
    - Naive32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
  grad.z
    = Naive32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
    - Naive32Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
  return 
    Naive32Volume1f_lerpf(volume,pos);
}

/*! 'Constructor' for a naive321uc volume.

  \note eventually this is supposed to return a "VolumePtr", but right
  now ISPC crashes when doing that */
uniform VolumePtr Naive32Volume1uc_create(uniform vec3i &size, const uint8 *uniform internal)
{
  uniform NaiveVolume *uniform volume = uniform new uniform NaiveVolume;
  uniform int64 numElements   = ((int64)size.x) * size.y * size.z;
  volume->inherited.transferFunction = NULL;
  volume->inherited.size     = size;
  volume->inherited.dataSize = numElements * sizeof(uniform int8);
  if (internal == NULL)
    volume->inherited.data = uniform new uniform int8[numElements];
  else
    volume->inherited.data = (void *uniform) internal;
  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  // volume->inherited.setRegion = Naive32Volume1uc_setRegion;
  volume->inherited.lerpf     = Naive32Volume1uc_lerpf;
  volume->inherited.gradf     = Naive32Volume1uc_gradf;
  return (uniform Volume *)volume;
}

#if 1
/*! 'Constructor' for a naive321f volume.

  \note eventually this is supposed to return a "VolumePtr", but right
  now ISPC crashes when doing that */
uniform VolumePtr Naive32Volume1f_create(uniform vec3i &size, const float *uniform internal)
{
  uniform NaiveVolume *uniform volume = uniform new uniform NaiveVolume;
  uniform int64 numElements   = ((int64)size.x) * size.y * size.z;
  volume->inherited.size     = size;
  volume->inherited.dataSize = numElements * sizeof(uniform float);
  if (volume->inherited.dataSize >= (1ULL<<32)) {
    print("ispc::Naive32Volume1f_create: size too big for 32-bit volume!");
    return NULL;
  }
  if (internal == NULL)
    volume->inherited.data     = uniform new uniform float[numElements];
  else
    volume->inherited.data = (void *uniform )internal;
  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  // volume->inherited.setRegion = Naive32Volume1f_setRegion;
  volume->inherited.lerpf     = Naive32Volume1f_lerpf;
  volume->inherited.gradf     = Naive32Volume1f_gradf;
  return (uniform Volume *)volume;
}


// ------------------------------------------------------------------
// publicly exported functions at the end
// ------------------------------------------------------------------

export void _Naive32Volume1uc_setRegion(uniform ExtVolumePtr _volume,
                                        const uniform vec3i &begin,
                                        const uniform vec3i &size,
                                        const uniform unsigned int8 *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  Naive32Volume1uc_setRegion(volume,begin,size,data);
}
export void _Naive32Volume1f_setRegion(uniform ExtVolumePtr _volume,
                                       const uniform vec3i &begin,
                                       const uniform vec3i &size,
                                       const uniform float *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  Naive32Volume1f_setRegion(volume,begin,size,data);
}

/* need this workaround because ISPC cannot export pointer to 'Volume'
   type because volume type has function pointers */
export  uniform ExtVolumePtr _Naive32Volume1uc_create(uniform vec3i &size, const uniform uint8 *uniform internal)
{ return (ExtVolumePtr)Naive32Volume1uc_create(size,internal); }
// extern "C" 
export uniform ExtVolumePtr _Naive32Volume1f_create(uniform vec3i &size, const uniform float *uniform internal)
{ return (ExtVolumePtr)Naive32Volume1f_create(size,internal); }




#endif
