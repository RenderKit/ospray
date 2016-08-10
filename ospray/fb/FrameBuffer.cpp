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

#include "FrameBuffer.h"
#include "FrameBuffer_ispc.h"
#ifdef OSPRAY_MPI
  #include "mpi/MPICommon.h"
#endif

namespace ospray {

  FrameBuffer::FrameBuffer(const vec2i &size,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer,
                           bool hasVarianceBuffer)
    : size(size),
      numTiles(divRoundUp(size, getTileSize())),
      maxValidPixelID(size-vec2i(1)),
      hasDepthBuffer(hasDepthBuffer),
      hasAccumBuffer(hasAccumBuffer),
      hasVarianceBuffer(hasVarianceBuffer),
      colorBufferFormat(colorBufferFormat),
      frameID(-1)
  {
    managedObjectType = OSP_FRAMEBUFFER;
    Assert(size.x > 0 && size.y > 0);
  }

  void FrameBuffer::beginFrame()
  {
    frameID++;
    ispc::FrameBuffer_set_frameID(getIE(), frameID);
  }

  /*! helper function for debugging. write out given pixels in PPM format */
  void writePPM(const std::string &fileName,
                const vec2i &size,
                const uint32 *pixels)
  {
    FILE *file = fopen(fileName.c_str(),"w");
    if (!file) {
      std::cout << "#osp:fb: could not open file " << fileName << std::endl;
      return;
    }
    fprintf(file,"P6\n%i %i\n255\n",size.x,size.y);
    for (int i=0;i<size.x*size.y;i++) {
      char *ptr = (char*)&pixels[i];
      fwrite(ptr,1,3,file);
    }
    fclose(file);
  }

  // helper function to write a (float) image as (flipped) PFM file
  void writePFM(const std::string &fileName,
                const vec2i &size,
                const int channel,
                const float *pixel)
  {
    FILE *file = fopen(fileName.c_str(),"w");
    if (!file) {
      std::cout << "#osp:fb: could not open file " << fileName << std::endl;
      return;
    }
    fprintf(file, "PF\n%i %i\n-1.0\n", size.x, size.y);
    float *out = STACK_BUFFER(float, 3*size.x);
    for (int y = 0; y < size.y; y++) {
      const float *in = (const float *)&pixel[(size.y-1-y)*size.x*channel];
      for (int x = 0; x < size.x; x++) {
        out[3*x + 0] = in[channel*x + 0];
        out[3*x + 1] = in[channel*x + 1];
        out[3*x + 2] = in[channel*x + 2];
      }
      fwrite(out, 3*size.x, sizeof(float), file);
    }
    fclose(file);
  }


  TileError::TileError(const vec2i &_numTiles)
    : numTiles(_numTiles)
    , tiles(_numTiles.x * _numTiles.y)
  {
    if (tiles > 0)
      tileErrorBuffer = (float*)alignedMalloc(sizeof(float) * tiles);
    else
      tileErrorBuffer = nullptr;

    // maximum number of regions: all regions are of size 3 are split in half
    errorRegion.reserve(divRoundUp(tiles * 2, 3));
  }

  TileError::~TileError()
  {
    alignedFree(tileErrorBuffer);
  }

  void TileError::clear()
  {
    for (int i = 0; i < tiles; i++)
      tileErrorBuffer[i] = inf;

    errorRegion.clear();
    // initially create one region covering the complete image
    errorRegion.push_back(box2i(vec2i(0), numTiles));
  }

  float TileError::operator[](const vec2i &tile) const
  {
    if (tiles <= 0)
      return inf;

    return tileErrorBuffer[tile.y * numTiles.x + tile.x];
  }

  void TileError::update(const vec2i &tile, const float err)
  {
    if (tiles > 0)
      tileErrorBuffer[tile.y * numTiles.x + tile.x] = err;
  }

#ifdef OSPRAY_MPI
  void TileError::sync()
  {
    if (tiles <= 0)
      return;

    int rc = MPI_Bcast(tileErrorBuffer, tiles, MPI_FLOAT, 0, MPI_COMM_WORLD); 
    mpi::checkMpiError(rc);
  }
#endif

  float TileError::refine(const float errorThreshold)
  {
    if (tiles <= 0)
      return inf;

    // process regions first, but don't process newly split regions again
    int regions = errorThreshold > 0.f ? errorRegion.size() : 0;
    for (int i = 0; i < regions; i++) {
      box2i& region = errorRegion[i];
      float err = 0.f;
      float maxErr = 0.0f;
      for (int y = region.lower.y; y < region.upper.y; y++)
        for (int x = region.lower.x; x < region.upper.x; x++) {
          int idx = y * numTiles.x + x;
          err += tileErrorBuffer[idx];
          maxErr = std::max(maxErr, tileErrorBuffer[idx]);
        }
      // set all tiles of this region to local max error to enforce their
      // refinement as a group
      for (int y = region.lower.y; y < region.upper.y; y++)
        for (int x = region.lower.x; x < region.upper.x; x++) {
          int idx = y * numTiles.x + x;
          tileErrorBuffer[idx] = maxErr;
        }
      vec2i size = region.size();
      int area = reduce_mul(size);
      err /= area; // avg
      if (err < 4.f*errorThreshold) { // split region?
        if (area <= 2) { // would just contain single tile after split: remove
          regions--;
          errorRegion[i] = errorRegion[regions];
          errorRegion[regions]= errorRegion.back();
          errorRegion.pop_back();
          i--;
          continue;
        }
        vec2i split = region.lower + size / 2; // TODO: find split with equal
        //       variance
        errorRegion.push_back(region); // region ref might become invalid
        if (size.x > size.y) {
          errorRegion[i].upper.x = split.x;
          errorRegion.back().lower.x = split.x;
        } else {
          errorRegion[i].upper.y = split.y;
          errorRegion.back().lower.y = split.y;
        }
      }
    }

    float maxErr = 0.0f;
    for (int i = 0; i < tiles; i++)
      maxErr = std::max(maxErr, tileErrorBuffer[i]);

    return maxErr;
  }
} // ::ospray
