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

#include "RawVolumeFile.h"

#include <cmath>
#include <stdio.h>
#include <string.h>

#include "common/FileName.h"

using ospcommon::FileName;

OSPVolume RawVolumeFile::importVolume(OSPVolume volume)
{
  // Look for the volume data file at the given path.
  FILE *file = NULL;
  FileName fn = filename;
  bool gzipped = fn.ext() == "gz";
  if (gzipped) {
    std::string cmd = "/usr/bin/gunzip -c "+filename;
    file = popen(cmd.c_str(),"r");
  } else {
    file = fopen(filename.c_str(),"rb");
  }
  //FILE *file = fopen(filename.c_str(), "rb");
  exitOnCondition(!file, "unable to open file '" + filename + "'");

  // Offset into the volume data file if any.
  int offset = 0;  
  ospGeti(volume, "filename offset", &offset);  fseek(file, offset, SEEK_SET);

  // Volume dimensions.
  ospcommon::vec3i volumeDimensions;  
  exitOnCondition(!ospGetVec3i(volume,
                               "dimensions",
                               &(osp::vec3i&)volumeDimensions),
                  "no volume dimensions specified");

  // Voxel type string.
  char *voxelType;  
  exitOnCondition(!ospGetString(volume, "voxelType", &voxelType),
                  "no voxel type specified");

  // Voxel size in bytes.
  size_t voxelSize;

  if (strcmp(voxelType, "uchar") == 0)
    voxelSize = sizeof(unsigned char);
  else if (strcmp(voxelType, "ushort") == 0)
    voxelSize = sizeof(uint16_t);
  else if (strcmp(voxelType, "float") == 0)
    voxelSize = sizeof(float);
  else if (strcmp(voxelType, "double") == 0)
    voxelSize = sizeof(double);
  else
    exitOnCondition(true, "unsupported voxel type");

  // Check if a subvolume of the volume has been specified.
  // Subvolume params: subvolumeOffsets, subvolumeDimensions, subvolumeSteps.
  // The subvolume defaults to full dimensions (allowing for just subsampling,
  // for example).
  ospcommon::vec3i subvolumeOffsets = ospcommon::vec3i(0);  
  ospGetVec3i(volume, "subvolumeOffsets", (osp::vec3i*)&subvolumeOffsets);
  exitOnCondition(reduce_min(subvolumeOffsets) < 0 ||
                  reduce_max(subvolumeOffsets - volumeDimensions) >= 0,
                  "invalid subvolume offsets");

  ospcommon::vec3i subvolumeDimensions = volumeDimensions - subvolumeOffsets;  
  ospGetVec3i(volume, "subvolumeDimensions", (osp::vec3i*)&subvolumeDimensions);
  exitOnCondition(reduce_min(subvolumeDimensions) < 1 ||
                  reduce_max(subvolumeDimensions -
                             (volumeDimensions - subvolumeOffsets)) > 0,
                  "invalid subvolume dimension(s) specified");

  ospcommon::vec3i subvolumeSteps = ospcommon::vec3i(1);  
  ospGetVec3i(volume, "subvolumeSteps", (osp::vec3i*)&subvolumeSteps);
  exitOnCondition(reduce_min(subvolumeSteps) < 1 ||
                  reduce_max(subvolumeSteps -
                             (volumeDimensions - subvolumeOffsets)) >= 0,
                  "invalid subvolume steps");

  bool useSubvolume = false;

  // The dimensions of the volume to be imported; this will be changed if a
  // subvolume is specified.
  ospcommon::vec3i importVolumeDimensions = volumeDimensions;

  if (reduce_max(subvolumeOffsets) > 0 ||
      subvolumeDimensions != volumeDimensions ||
      reduce_max(subvolumeSteps) > 1) {

    useSubvolume = true;

    // The dimensions of the volume to be imported, considering the subvolume
    // specified.
    int xdim = subvolumeDimensions.x / subvolumeSteps.x +
        (subvolumeDimensions.x % subvolumeSteps.x != 0);
    int ydim = subvolumeDimensions.y / subvolumeSteps.y +
        (subvolumeDimensions.y % subvolumeSteps.y != 0);
    int zdim = subvolumeDimensions.z / subvolumeSteps.z +
        (subvolumeDimensions.z % subvolumeSteps.z != 0);
    importVolumeDimensions = ospcommon::vec3i(xdim, ydim, zdim);

    // Range check.
    exitOnCondition(reduce_min(importVolumeDimensions) <= 0,
                    "invalid import volume dimensions");

    // Update the provided dimensions of the volume for the subvolume specified.
    ospSetVec3i(volume, "dimensions", (osp::vec3i&)importVolumeDimensions);
  }

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
  ospcommon::vec2f voxelRange(+std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
#endif

  // To avoid hitting memory limits or exceeding the 2GB limit in MPIDevice::ospSetRegion we
  // set the volume data in at 1.5GB chunks
  // TODO How to compute these chunks, they must be convex as well, e.g. we can't set
  // 2.5 scanlines of the data b/c of the params we give to setRegion are the start & size of the chunk.
  // For testing try with super tiny 1k chunks
  const int SET_REGION_CHUNK_SIZE = 1512e6;
  const int MAX_CHUNK_VOXELS = SET_REGION_CHUNK_SIZE / voxelSize;
  // For chunk dims we must step biggest along X until we hit chunkDim.x == volumeDimensions.x
  // then increase chunk size along Y until we hit chunkDim.y == volumeDimensions.y and then
  // we can increase chunk size along Z (assumes row order is XYZ which should be fine for any sane raw file)
  osp::vec3i chunkDimensions;
  chunkDimensions.x = MAX_CHUNK_VOXELS;
  chunkDimensions.y = 1;
  chunkDimensions.z = 1;
  if (chunkDimensions.x > volumeDimensions.x) {
    chunkDimensions.x = volumeDimensions.x;
    chunkDimensions.y = MAX_CHUNK_VOXELS / chunkDimensions.x;
    if (chunkDimensions.y > volumeDimensions.y) {
      chunkDimensions.y = volumeDimensions.y;
      chunkDimensions.z = std::min(volumeDimensions.z, MAX_CHUNK_VOXELS / (chunkDimensions.x * chunkDimensions.y));
    }
  }

  std::cout << "Reading volume in chunks of size {" << chunkDimensions.x << ", " << chunkDimensions.y
    << ", " << chunkDimensions.z << "}" << std::endl;

  if (!useSubvolume) {
    // Allocate memory for a single chunk
    const size_t chunkVoxels = chunkDimensions.x * chunkDimensions.y * chunkDimensions.z;
    unsigned char *voxelData = new unsigned char[chunkVoxels * voxelSize];
    osp::vec3i numChunks;
    numChunks.x = volumeDimensions.x / chunkDimensions.x;
    numChunks.y = volumeDimensions.y / chunkDimensions.y;
    numChunks.z = volumeDimensions.z / chunkDimensions.z;
    osp::vec3i remainderVoxels;
    remainderVoxels.x = volumeDimensions.x % chunkDimensions.x;
    remainderVoxels.y = volumeDimensions.y % chunkDimensions.y;
    remainderVoxels.z = volumeDimensions.z % chunkDimensions.z;
    std::cout << "Number of chunks on each axis = {" << numChunks.x << ", " << numChunks.y << ", "
      << numChunks.z << "}, remainderVoxels {" << remainderVoxels.x
      << ", " << remainderVoxels.y << ", " << remainderVoxels.z << "}, each chunk is "
      << chunkVoxels << " voxels " << std::endl;
    // Load and copy in each chunk of the volume data into the OSPRay volume
    for (int chunkz = 0; chunkz < numChunks.z; ++chunkz) {
      for (int chunky = 0; chunky < numChunks.y; ++chunky) {
        for (int chunkx = 0; chunkx < numChunks.x; ++chunkx) {
          size_t voxelsRead = fread(voxelData, voxelSize, chunkVoxels, file);
          // The end of the file may have been reached unexpectedly.
          exitOnCondition(voxelsRead != chunkVoxels, "end of volume file reached before read completed");

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
          extendVoxelRange(voxelRange, voxelSize, voxelData, voxelsRead);
#endif
          ospcommon::vec3i region_lo(chunkx * chunkDimensions.x, chunky * chunkDimensions.y,
              chunkz * chunkDimensions.z);

          ospSetRegion(volume, voxelData, (osp::vec3i&)region_lo, chunkDimensions);
        }
        // Read any remainder voxels on the scanline
        if (remainderVoxels.x > 0) {
          // We should only have remainder along x if we couldn't fit a scanline in SET_REGION_CHUNK_SIZE
          assert(chunkDimensions.y == 1 && chunkDimensions.z == 1);
          size_t remainder = remainderVoxels.x;
          size_t voxelsRead = fread(voxelData, voxelSize, remainder, file);
          ospcommon::vec3i region_lo(numChunks.x * chunkDimensions.x, chunky * chunkDimensions.y,
              chunkz * chunkDimensions.z);
          ospcommon::vec3i region_sz(remainderVoxels.x, chunkDimensions.y, chunkDimensions.z);

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
          extendVoxelRange(voxelRange, voxelSize, voxelData, voxelsRead);
#endif
          ospSetRegion(volume, voxelData, (osp::vec3i&)region_lo, (osp::vec3i&)region_sz);
        }
      }
      if (remainderVoxels.y > 0) {
        // We should only have remainder along y if we couldn't fit a slice in SET_REGION_CHUNK_SIZE
        assert(chunkDimensions.x == volumeDimensions.x && chunkDimensions.z == 1);
        size_t remainder = chunkDimensions.x * remainderVoxels.y;
        size_t voxelsRead = fread(voxelData, voxelSize, remainder, file);
        ospcommon::vec3i region_lo(0, numChunks.y * chunkDimensions.y,
            chunkz * chunkDimensions.z);
        ospcommon::vec3i region_sz(chunkDimensions.x, remainderVoxels.y, chunkDimensions.z);

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
        extendVoxelRange(voxelRange, voxelSize, voxelData, voxelsRead);
#endif

        ospSetRegion(volume, voxelData, (osp::vec3i&)region_lo, (osp::vec3i&)region_sz);
      }
    }
    if (remainderVoxels.z > 0) {
      // We should only have remainder along z if we couldn't fit the volume in SET_REGION_CHUNK_SIZE
      assert(chunkDimensions.x == volumeDimensions.x && chunkDimensions.y == volumeDimensions.y);
      size_t remainder = chunkDimensions.x * chunkDimensions.y * remainderVoxels.z;
      size_t voxelsRead = fread(voxelData, voxelSize, remainder, file);
      ospcommon::vec3i region_lo(0, 0, numChunks.z * chunkDimensions.z);
      ospcommon::vec3i region_sz(chunkDimensions.x, chunkDimensions.y, remainderVoxels.z);

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
      extendVoxelRange(voxelRange, voxelSize, voxelData, voxelsRead);
#endif

      ospSetRegion(volume, voxelData, (osp::vec3i&)region_lo, (osp::vec3i&)region_sz);
    }
    delete[] voxelData;
  } else {

#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    throw std::runtime_error("computation of voxel range not yet "
                             "implemented for subvolumes");
#endif

    // Allocate memory for a single row of voxel data.
    unsigned char *rowData = new unsigned char[volumeDimensions.x * voxelSize];

    // Allocate memory for a single row of voxel data for the subvolume.
    unsigned char *subvolumeRowData =
        new unsigned char[importVolumeDimensions.x * voxelSize];

    // Read the subvolume data from the full volume.
    for(long i3 = subvolumeOffsets.z;
        i3 < subvolumeOffsets.z + subvolumeDimensions.z;
        i3 += subvolumeSteps.z) {

      for(long i2 = subvolumeOffsets.y;
          i2 < subvolumeOffsets.y + subvolumeDimensions.y;
          i2+=subvolumeSteps.y) {

        // Seek to appropriate location in file.
        fseek(file,
              offset +
              (i3 * volumeDimensions.y * volumeDimensions.x * voxelSize) +
              (i2 * volumeDimensions.x * voxelSize), SEEK_SET);

        // Read row from volume.
        size_t voxelsRead = fread(rowData, voxelSize, volumeDimensions.x, file);

        // The end of the file may have been reached unexpectedly.
        exitOnCondition(voxelsRead != volumeDimensions.x,
                        "end of volume file reached before read completed");

        // Resample row for the subvolume.
        for(long i1 = subvolumeOffsets.x;
            i1 < subvolumeOffsets.x+subvolumeDimensions.x;
            i1 += subvolumeSteps.x) {
          memcpy(&subvolumeRowData[(i1 - subvolumeOffsets.x) /
                                    subvolumeSteps.x * voxelSize],
                 &rowData[i1 * voxelSize],
                 voxelSize);
        }

        // Copy subvolume row into the volume.
        ospcommon::vec3i region_lo(0, 
                                (i2 - subvolumeOffsets.y) / subvolumeSteps.y, 
                                (i3 - subvolumeOffsets.z) / subvolumeSteps.z);
        ospcommon::vec3i region_sz(importVolumeDimensions.x, 1, 1);
        ospSetRegion(volume,
                     &subvolumeRowData[0],
                     (osp::vec3i&)region_lo,
                     (osp::vec3i&)region_sz);
      }
    }

    // Clean up.
    delete [] rowData;
    delete [] subvolumeRowData;
  }

  if (gzipped)
    pclose(file);
  else
    fclose(file);
  // Return the volume.
  
#ifdef OSPRAY_VOLUME_VOXELRANGE_IN_APP
  VolumeFile::voxelRangeOf[volume] = voxelRange;
#endif
  
  return(volume);
}
