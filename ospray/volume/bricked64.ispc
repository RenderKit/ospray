/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "volume.ih"
#include "bricked64.ih"

// ------------------------------------------------------------------
// the data structure itself
// ------------------------------------------------------------------

#define CELL_BITS_MASK ((1<<((CACHE_BRICK_BITS)))-1)
#define BRICK_BITS_MASK ((1<<((PAGE_BRICK_BITS-CACHE_BRICK_BITS)))-1)

#define CELLS_PER_BRICK (1<<(3*CACHE_BRICK_BITS))
#define BRICKS_PER_PAGE (1<<(3*(PAGE_BRICK_BITS-CACHE_BRICK_BITS)))

extern "C" void cSideLargeMemMalloc(void *uniform *uniform ptr,
                                    uniform int64 *uniform numBytes);

struct PBBrick1f {
  float cell[CELLS_PER_BRICK];
};

struct PBPage1f {
  PBBrick1f brick[BRICKS_PER_PAGE];
};

struct PBVolume1f {
  Volume inherited;
  vec3ui numPages;
  uniform PBPage1f *page;
};

struct PBBrick1uc {
  uint8 cell[CELLS_PER_BRICK];
};

struct PBPage1uc {
  PBBrick1uc brick[BRICKS_PER_PAGE];
};

struct PBVolume1uc {
  Volume inherited;
  vec3ui numPages;
  uniform PBPage1uc *page;
};

#define NOTIMPLEMENTED print("NOT IMPLEMENTED\n");

// ------------------------------------------------------------------
// 'internal' functions
// ------------------------------------------------------------------

inline void PBVolume1f_getCoord(uint32 &pageID,
                                uint32 &brickID,
                                uint32 &cellID,
                                uniform PBVolume1f *uniform volume,
                                const vec3ui &pos)
{
  const uint32 page_x = pos.x >> PAGE_BRICK_BITS;
  const uint32 page_y = pos.y >> PAGE_BRICK_BITS;
  const uint32 page_z = pos.z >> PAGE_BRICK_BITS;
  pageID
    = page_x+volume->numPages.x*(page_y+volume->numPages.y*page_z);

  // print("page_x %\n",page_x);
  // print("page_y %\n",page_y);
  // print("page_z %\n",page_z);

  const uint32 brick_x = (pos.x >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  const uint32 brick_y = (pos.y >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  const uint32 brick_z = (pos.z >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  brickID
    = (brick_x)
    + (brick_y << (PAGE_BRICK_BITS-CACHE_BRICK_BITS))
    + (brick_z << (2*(PAGE_BRICK_BITS-CACHE_BRICK_BITS)));

  // print("brick_x %\n",brick_x);
  // print("brick_y %\n",brick_y);
  // print("brick_z %\n",brick_z);


  const uint32 cell_x = pos.x & CELL_BITS_MASK;
  const uint32 cell_y = pos.y & CELL_BITS_MASK;
  const uint32 cell_z = pos.z & CELL_BITS_MASK;
  cellID
    = (cell_x)
    + (cell_y << (CACHE_BRICK_BITS))
    + (cell_z << (2*(CACHE_BRICK_BITS)));

  // print("cell_x %\n",cell_x);
  // print("cell_y %\n",cell_y);
  // print("cell_z %\n",cell_z);


  // foreach_unique(pID in pageID) {
  //   uniform PBPage1f *uniform page = &volume->page[pID];
  //   return page->brick[brickID].cell[cellID];
  // }
}


inline void PBVolume1uc_getCoord(uint32 &pageID,
                                 uint32 &brickID,
                                 uint32 &cellID,
                                 uniform PBVolume1uc *uniform volume,
                                 const vec3ui &pos)
{
  const uint32 page_x = pos.x >> PAGE_BRICK_BITS;
  const uint32 page_y = pos.y >> PAGE_BRICK_BITS;
  const uint32 page_z = pos.z >> PAGE_BRICK_BITS;
  pageID
    = page_x+volume->numPages.x*(page_y+volume->numPages.y*page_z);

  const uint32 brick_x = (pos.x >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  const uint32 brick_y = (pos.y >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  const uint32 brick_z = (pos.z >> CACHE_BRICK_BITS) & BRICK_BITS_MASK;
  brickID
    = (brick_x)
    + (brick_y << (PAGE_BRICK_BITS-CACHE_BRICK_BITS))
    + (brick_z << (2*(PAGE_BRICK_BITS-CACHE_BRICK_BITS)));

  const uint32 cell_x = pos.x & CELL_BITS_MASK;
  const uint32 cell_y = pos.y & CELL_BITS_MASK;
  const uint32 cell_z = pos.z & CELL_BITS_MASK;
  cellID
    = (cell_x)
    + (cell_y << (CACHE_BRICK_BITS))
    + (cell_z << (2*(CACHE_BRICK_BITS)));
}



// /*! set given region to voxel data specified in 'data'. The region
//     *must* be inside the volume, and the 'data' type must match the
//     internal data format */
// void PBVolume1f_setRegion(uniform VolumePtr _volume,
//                           const uniform vec3i &begin,
//                           const uniform vec3i &size,
//                           const void *uniform data)
// {
//   // uniform PBVolume1f *uniform volume 
//   //   = (uniform PBVolume1f *uniform)_volume;

//   // unsigned int8 *uniform dst = (unsigned int8 *uniform)volume->inherited.data;
//   // unsigned int8 *uniform src = (unsigned int8 *uniform)data;
//   // foreach (sz = 0 ... size.z, 
//   //          sy = 0 ... size.y, 
//   //          sx = 0 ... size.x) 
//   //   {
//   //     const int src_idx = sx + size.x * (sy + size.y * sz);

//   //     const int dx = begin.x + sx;
//   //     const int dy = begin.y + sy;
//   //     const int dz = begin.z + sz;
//   //     const vec3ui pos = make_vec3ui(dx,dy,dz);
//   //     uint32 pageID, brickID, cellID;
//   //     PBVolume1f_getCoord(pageID,brickID,cellID,
//   //                         volume,pos);
//   //     foreach_unique(pID in pageID) {
//   //       uniform PBPage1f *uniform page = &volume->page[pID];
//   //       page->brick[brickID].cell[cellID] = src[src_idx];
//   //     }
//   //   }
// }

void PBVolume1f_setRegion(uniform VolumePtr _volume,             
                          const uniform vec3i &begin,                 
                          const uniform vec3i &size,                  
                          const void *uniform data)
{
  uniform PBVolume1f *uniform volume 
    = (uniform PBVolume1f *uniform)_volume;

  // unsigned float *uniform dst = (unsigned float *uniform)volume->inherited.data;
  uniform float *uniform src = (uniform float *uniform)data;
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int src_idx = sx + size.x * (sy + size.y * sz);

      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const vec3ui pos = make_vec3ui(dx,dy,dz);
      uint32 pageID, brickID, cellID;
      PBVolume1f_getCoord(pageID,brickID,cellID,
                          volume,pos);
      foreach_unique(pID in pageID) {
        uniform PBPage1f *uniform page = &volume->page[pID];
        page->brick[brickID].cell[cellID] = src[src_idx];
      }
    }
}


void PBVolume1uc_setRegion(uniform VolumePtr _volume,             
                           const uniform vec3i &begin,                 
                           const uniform vec3i &size,                  
                           const void *uniform data)
{
  uniform PBVolume1uc *uniform volume 
    = (uniform PBVolume1uc *uniform)_volume;
  
  // unsigned float *uniform dst = (unsigned float *uniform)volume->inherited.data;
  uniform uint8 *uniform src = (uniform uint8 *uniform)data;
  foreach (sz = 0 ... size.z, 
           sy = 0 ... size.y, 
           sx = 0 ... size.x) 
    {
      const int src_idx = sx + size.x * (sy + size.y * sz);

      const int dx = begin.x + sx;
      const int dy = begin.y + sy;
      const int dz = begin.z + sz;
      const vec3ui pos = make_vec3ui(dx,dy,dz);
      uint32 pageID, brickID, cellID;
      PBVolume1uc_getCoord(pageID,brickID,cellID,
                           volume,pos);
      foreach_unique(pID in pageID) {
        uniform PBPage1uc *uniform page = &volume->page[pID];
        page->brick[brickID].cell[cellID] = src[src_idx];
      }
    }
}




/*! get voxel for bricked volume with 1uc data format and 64-bit addressing */
inline float PBVolume1uc_getVoxel(uniform PBVolume1uc *uniform volume,
                                 const vec3ui &coord)
{
  uint32 pageID, brickID, cellID;
  PBVolume1uc_getCoord(pageID,brickID,cellID,
                       volume,coord);
  
  // print("IDs % % %\n",pageID,brickID,cellID);
  float res;
  foreach_unique(pID in pageID) {
    uniform PBPage1uc *uniform page = &volume->page[pID];
    res = page->brick[brickID].cell[cellID];
  }
  return res;
}

/*! get voxel for bricked volume with 1uc data format and 64-bit addressing */
inline float PBVolume1f_getVoxel(uniform PBVolume1f *uniform volume,
                                 const vec3ui &coord)
{
  uint32 pageID, brickID, cellID;
  PBVolume1f_getCoord(pageID,brickID,cellID,
                      volume,coord);

  float res;
  foreach_unique(pID in pageID) {
    uniform PBPage1f *uniform page = &volume->page[pID];
    res = page->brick[brickID].cell[cellID];
  }
  return res;
}

// float gatherFloat(const uniform float *uniform data, const int idx)
// {
//   // print("BEFORE");
//   const float f = data[idx];;
//   print("ab");
//   return f;
// }

// /*! get voxel for bricked volume with 1uc data format and 64-bit addressing */
// inline 
// float getVoxel1f64(uniform PagedBrickedVolume *uniform volume,
//                    const vec3i &coord)
// {
//   const uniform float *uniform data
//     = (const uniform float *uniform )volume->inherited.data;
//   const int idx = PagedBrickedVolume_getCoord(volume,coord);
//   return data[idx];
// }

// varying float PagedBricked64Volume1uc_lerpf(uniform VolumePtr _volume,            
//                                        const varying vec3f &pos)
// {
//   uniform PagedBrickedVolume *uniform volume = (uniform PagedBrickedVolume *uniform)_volume;
//   const float clamped_x = clamp(pos.x,0.f,volume->inherited.clampSize.x);
//   const float clamped_y = clamp(pos.y,0.f,volume->inherited.clampSize.y);
//   const float clamped_z = clamp(pos.z,0.f,volume->inherited.clampSize.z);

//   const int32 x0 = (int)(clamped_x);
//   const int32 y0 = (int)(clamped_y);
//   const int32 z0 = (int)(clamped_z);

//   const float fx = clamped_x - (float)x0;
//   const float fy = clamped_y - (float)y0;
//   const float fz = clamped_z - (float)z0;

//   const uint32 x1 = x0 + 1;
//   const uint32 y1 = y0 + 1;
//   const uint32 z1 = z0 + 1;

//   const float v000 = getVoxel1uc64(volume,make_vec3i(x0,y0,z0));
//   const float v001 = getVoxel1uc64(volume,make_vec3i(x1,y0,z0));
//   const float v010 = getVoxel1uc64(volume,make_vec3i(x0,y1,z0));
//   const float v011 = getVoxel1uc64(volume,make_vec3i(x1,y1,z0));
//   const float v100 = getVoxel1uc64(volume,make_vec3i(x0,y0,z1));
//   const float v101 = getVoxel1uc64(volume,make_vec3i(x1,y0,z1));
//   const float v110 = getVoxel1uc64(volume,make_vec3i(x0,y1,z1));
//   const float v111 = getVoxel1uc64(volume,make_vec3i(x1,y1,z1));
//   //#endif
//   // print("bla\n");  
//   const float v00 = v000 + fx * (v001-v000);
//   const float v01 = v010 + fx * (v011-v010);
//   const float v10 = v100 + fx * (v101-v100);
//   const float v11 = v110 + fx * (v111-v110);

//   const float v0 = v00 + fy * (v01-v00);
//   const float v1 = v10 + fy * (v11-v10);

//   const float v = v0 + fz * (v1-v0);

//   return v * (1.f/255.f);
// }

varying float 
PagedBricked64Volume1f_lerpf(uniform VolumePtr _volume,            
                             const varying vec3f &pos)
{
  uniform PBVolume1f *uniform volume
    = (uniform PBVolume1f *uniform)_volume;
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
  
  const float v000 = PBVolume1f_getVoxel(volume,make_vec3ui(x0,y0,z0));
  const float v001 = PBVolume1f_getVoxel(volume,make_vec3ui(x1,y0,z0));
  const float v010 = PBVolume1f_getVoxel(volume,make_vec3ui(x0,y1,z0));
  const float v011 = PBVolume1f_getVoxel(volume,make_vec3ui(x1,y1,z0));
  const float v100 = PBVolume1f_getVoxel(volume,make_vec3ui(x0,y0,z1));
  const float v101 = PBVolume1f_getVoxel(volume,make_vec3ui(x1,y0,z1));
  const float v110 = PBVolume1f_getVoxel(volume,make_vec3ui(x0,y1,z1));
  const float v111 = PBVolume1f_getVoxel(volume,make_vec3ui(x1,y1,z1));

  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  return v;
}

varying float 
PagedBricked64Volume1uc_lerpf(uniform VolumePtr _volume,            
                             const varying vec3f &pos)
{
  uniform PBVolume1uc *uniform volume
    = (uniform PBVolume1uc *uniform)_volume;
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
  
  const float v000 = PBVolume1uc_getVoxel(volume,make_vec3ui(x0,y0,z0));
  const float v001 = PBVolume1uc_getVoxel(volume,make_vec3ui(x1,y0,z0));
  const float v010 = PBVolume1uc_getVoxel(volume,make_vec3ui(x0,y1,z0));
  const float v011 = PBVolume1uc_getVoxel(volume,make_vec3ui(x1,y1,z0));
  const float v100 = PBVolume1uc_getVoxel(volume,make_vec3ui(x0,y0,z1));
  const float v101 = PBVolume1uc_getVoxel(volume,make_vec3ui(x1,y0,z1));
  const float v110 = PBVolume1uc_getVoxel(volume,make_vec3ui(x0,y1,z1));
  const float v111 = PBVolume1uc_getVoxel(volume,make_vec3ui(x1,y1,z1));

  const float v00 = v000 + fx * (v001-v000);
  const float v01 = v010 + fx * (v011-v010);
  const float v10 = v100 + fx * (v101-v100);
  const float v11 = v110 + fx * (v111-v110);

  const float v0 = v00 + fy * (v01-v00);
  const float v1 = v10 + fy * (v11-v10);

  const float v = v0 + fz * (v1-v0);

  return v * (1.f/255.f);
}

// varying float PagedBricked64Volume1uc_gradf(uniform VolumePtr volume,            
//                                     varying vec3f &grad,            
//                                     const varying vec3f &pos)
// {
//   grad.x
//     = PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
//     - PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
//   grad.y
//     = PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
//     - PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
//   grad.z
//     = PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
//     - PagedBricked64Volume1uc_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
//   return 
//     PagedBricked64Volume1uc_lerpf(volume,pos);
// }
// varying float PagedBricked64Volume1f_gradf(uniform VolumePtr volume,            
//                                     varying vec3f &grad,            
//                                     const varying vec3f &pos)
// {
//   grad.x
//     = PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x+1.f,pos.y,pos.z))
//     - PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x-1.f,pos.y,pos.z));
//   grad.y
//     = PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y+1.f,pos.z))
//     - PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y-1.f,pos.z));
//   grad.z
//     = PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z+1.f))
//     - PagedBricked64Volume1f_lerpf(volume,make_vec3f(pos.x,pos.y,pos.z-1.f));
//   return 
//     PagedBricked64Volume1f_lerpf(volume,pos);
// }

inline uniform int divRoundUp(uniform int nom, uniform int den)
{
  return (nom + den - 1)/den;
}


/* need this workaround because ISPC cannot export pointer to 'Volume'
   type because volume type has function pointers */
export void *uniform PagedBrickedVolume1f_create(uniform vec3ui &size, 
                                                 const float *uniform internal)
{ 
  uniform PBVolume1f *uniform volume = uniform new uniform PBVolume1f;
  const uniform int pageRes = 1<<PAGE_BRICK_BITS;
  volume->numPages.x = divRoundUp(size.x,pageRes);
  volume->numPages.y = divRoundUp(size.y,pageRes);
  volume->numPages.z = divRoundUp(size.z,pageRes);
  const uniform int64 numPages
    = ((int64)volume->numPages.x) * volume->numPages.y * volume->numPages.z;
  volume->inherited.transferFunction = NULL;
  volume->inherited.size     = make_vec3i(size);
  volume->inherited.dataSize = numPages * sizeof(PBPage1f);
  if (internal)
    volume->inherited.data = (void *uniform) internal;
  else 
    volume->inherited.data     = uniform new uniform PBPage1f[numPages];
  volume->page = (uniform PBPage1f*)volume->inherited.data;

  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  
  volume->inherited.lerpf     = PagedBricked64Volume1f_lerpf;
  // volume->inherited.gradf     = PagedBricked64Volume1f_gradf;
  return (uniform VolumePtr)volume;
}

export void *uniform PagedBricked64Volume1uc_create(uniform vec3i &size, 
                                                    const uint8 *uniform internal)
{
  uniform PBVolume1uc *uniform volume = uniform new uniform PBVolume1uc;
  const uniform int pageRes = 1<<PAGE_BRICK_BITS;
  volume->numPages.x = divRoundUp(size.x,pageRes);
  volume->numPages.y = divRoundUp(size.y,pageRes);
  volume->numPages.z = divRoundUp(size.z,pageRes);
  const uniform int64 numPages
    = ((int64)volume->numPages.x) * volume->numPages.y * volume->numPages.z;
  volume->inherited.transferFunction = NULL;
  volume->inherited.size     = make_vec3i(size);
  volume->inherited.dataSize = numPages * sizeof(PBPage1uc);
  if (internal)
    volume->inherited.data = (void *uniform) internal;
  else {
    uniform int64 numBytes = (uniform int64)(numPages)*sizeof(uniform PBPage1uc);
    print("#osp/volume/bricked64: allocating % pages (total size %)\n",numPages,numBytes);
#if 1
    cSideLargeMemMalloc(&volume->inherited.data,&numBytes);
    // volume->inherited.data     
    //   = (uniform PBPage1uc *uniform)
#else
    volume->inherited.data     
      = uniform new uniform PBPage1uc[numPages];
#endif
    print("#osp/volume/bricked64: got pointer %\n",volume->inherited.data);
    // for (uniform int64 i=0;i<numBytes;i+=4*1024) {
    //   ((uniform uint8*)volume->inherited.data)[i] = 0;
    // }
  }
  print("#osp:volume/bricked64: allocated bricking volume\n");
  volume->page = (uniform PBPage1uc*)volume->inherited.data;

  volume->inherited.clampSize.x = biggestFloatSmallerThan(size.x-1);
  volume->inherited.clampSize.y = biggestFloatSmallerThan(size.y-1);
  volume->inherited.clampSize.z = biggestFloatSmallerThan(size.z-1);
  
  volume->inherited.lerpf     = PagedBricked64Volume1uc_lerpf;
  // volume->inherited.gradf     = PagedBricked64Volume1uc_gradf;
  return (uniform VolumePtr)volume;
}

export void PagedBricked64Volume1f_setRegion(void *uniform _volume,
                                         const uniform vec3i &begin,
                                         const uniform vec3i &size,
                                         const uniform float *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  
  PBVolume1f_setRegion(volume,begin,size,data);
}

export void PagedBricked64Volume1uc_setRegion(void *uniform _volume,
                                              const uniform vec3i &begin,
                                              const uniform vec3i &size,
                                              const uniform uint8 *uniform data)
{ 
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  PBVolume1uc_setRegion(volume,begin,size,data);
}


/* need this workaround because ISPC cannot export pointer to 'Volume'
   type because volume type has function pointers */
// export uniform ExtVolumePtr _PagedBricked64Volume1uc_create(uniform vec3i &size, const uint8 *uniform internal)
// {
//   NOTIMPLENTED;
// }

