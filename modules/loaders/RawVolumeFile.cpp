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

#include <stdio.h>
#include <string.h>
#include "modules/loaders/RawVolumeFile.h"
#include "common/sys/filename.h"

OSPVolume RawVolumeFile::importVolume(OSPVolume volume)
{
  // Look for the volume data file at the given path.
  FILE *file = NULL;
  embree::FileName fn = filename;
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
  int offset = 0;  ospGeti(volume, "filename offset", &offset);  fseek(file, offset, SEEK_SET);

  // Volume dimensions.
  osp::vec3i volumeDimensions;  exitOnCondition(!ospGetVec3i(volume, "dimensions", &volumeDimensions), "no volume dimensions specified");

  // Voxel type string.
  char *voxelType;  exitOnCondition(!ospGetString(volume, "voxelType", &voxelType), "no voxel type specified");

  // Voxel size in bytes.
  size_t voxelSize;

  if (strcmp(voxelType, "uchar") == 0)
    voxelSize = sizeof(unsigned char);
  else if (strcmp(voxelType, "float") == 0)
    voxelSize = sizeof(float);
  else if (strcmp(voxelType, "double") == 0)
    voxelSize = sizeof(double);
  else
    exitOnCondition(true, "unsupported voxel type");

  // Check if a subvolume of the volume has been specified.
  // Subvolume parameters: subvolumeOffsets, subvolumeDimensions, subvolumeSteps.
  // The subvolume defaults to full dimensions (allowing for just subsampling, for example).
  osp::vec3i subvolumeOffsets = osp::vec3i(0);  ospGetVec3i(volume, "subvolumeOffsets", &subvolumeOffsets);
  exitOnCondition(reduce_min(subvolumeOffsets) < 0 || reduce_max(subvolumeOffsets - volumeDimensions) >= 0, "invalid subvolume offsets");

  osp::vec3i subvolumeDimensions = volumeDimensions - subvolumeOffsets;  ospGetVec3i(volume, "subvolumeDimensions", &subvolumeDimensions);
  exitOnCondition(reduce_min(subvolumeDimensions) < 1 || reduce_max(subvolumeDimensions - (volumeDimensions - subvolumeOffsets)) > 0, "invalid subvolume dimension(s) specified");

  osp::vec3i subvolumeSteps = osp::vec3i(1);  ospGetVec3i(volume, "subvolumeSteps", &subvolumeSteps);
  exitOnCondition(reduce_min(subvolumeSteps) < 1 || reduce_max(subvolumeSteps - (volumeDimensions - subvolumeOffsets)) >= 0, "invalid subvolume steps");

  bool useSubvolume = false;

  // The dimensions of the volume to be imported; this will be changed if a subvolume is specified.
  osp::vec3i importVolumeDimensions = volumeDimensions;

  if (reduce_max(subvolumeOffsets) > 0 || subvolumeDimensions != volumeDimensions || reduce_max(subvolumeSteps) > 1) {

    useSubvolume = true;

    // The dimensions of the volume to be imported, considering the subvolume specified.
    importVolumeDimensions = osp::vec3i(subvolumeDimensions.x / subvolumeSteps.x + (subvolumeDimensions.x % subvolumeSteps.x != 0),
                                        subvolumeDimensions.y / subvolumeSteps.y + (subvolumeDimensions.y % subvolumeSteps.y != 0),
                                        subvolumeDimensions.z / subvolumeSteps.z + (subvolumeDimensions.z % subvolumeSteps.z != 0));

    // Range check.
    exitOnCondition(reduce_min(importVolumeDimensions) <= 0, "invalid import volume dimensions");

    // Update the provided dimensions of the volume for the subvolume specified.
    ospSetVec3i(volume, "dimensions", importVolumeDimensions);
  }

  if (!useSubvolume) {

    // Voxel count.
    size_t voxelCount = volumeDimensions.x * volumeDimensions.y;

    // Allocate memory for a single slice through the volume.
    unsigned char *voxelData = new unsigned char[voxelCount * voxelSize];

    // We copy data into the volume by the slice in case memory is limited.
    for (size_t z=0 ; z < volumeDimensions.z ; z++) {

      // Copy voxel data into the buffer.
      size_t voxelsRead = fread(voxelData, voxelSize, voxelCount, file);

      // The end of the file may have been reached unexpectedly.
      exitOnCondition(voxelsRead != voxelCount, "end of volume file reached before read completed");

      // Copy the voxels into the volume.
      ospSetRegion(volume, voxelData, osp::vec3i(0, 0, z), osp::vec3i(volumeDimensions.x, volumeDimensions.y, 1));
    }

    // Clean up.
    delete [] voxelData;

  } else {

    // Allocate memory for a single row of voxel data.
    unsigned char *rowData = new unsigned char[volumeDimensions.x * voxelSize];

    // Allocate memory for a single row of voxel data for the subvolume.
    unsigned char *subvolumeRowData = new unsigned char[importVolumeDimensions.x * voxelSize];

    // Read the subvolume data from the full volume.
    for(long i3=subvolumeOffsets.z; i3<subvolumeOffsets.z+subvolumeDimensions.z; i3+=subvolumeSteps.z) {

      for(long i2=subvolumeOffsets.y; i2<subvolumeOffsets.y+subvolumeDimensions.y; i2+=subvolumeSteps.y) {

        // Seek to appropriate location in file.
        fseek(file, offset + i3 * volumeDimensions.y * volumeDimensions.x * voxelSize + i2 * volumeDimensions.x * voxelSize, SEEK_SET);

        // Read row from volume.
        size_t voxelsRead = fread(rowData, voxelSize, volumeDimensions.x, file);

        // The end of the file may have been reached unexpectedly.
        exitOnCondition(voxelsRead != volumeDimensions.x, "end of volume file reached before read completed");

        // Resample row for the subvolume.
        for(long i1=subvolumeOffsets.x; i1<subvolumeOffsets.x+subvolumeDimensions.x; i1+=subvolumeSteps.x)
          memcpy(&subvolumeRowData[(i1 - subvolumeOffsets.x) / subvolumeSteps.x * voxelSize], &rowData[i1 * voxelSize], voxelSize);

        // Copy subvolume row into the volume.
        ospSetRegion(volume, &subvolumeRowData[0], osp::vec3i(0, (i2 - subvolumeOffsets.y) / subvolumeSteps.y, (i3 - subvolumeOffsets.z) / subvolumeSteps.z), osp::vec3i(importVolumeDimensions.x, 1, 1));
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
  return(volume);
}
