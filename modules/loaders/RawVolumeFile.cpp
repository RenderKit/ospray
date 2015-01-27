// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

OSPVolume RawVolumeFile::importVolume(OSPVolume volume) {

  //! Look for the volume data file at the given path.
  FILE *file = fopen(filename.c_str(), "rb");  exitOnCondition(!file, "unable to open file '" + filename + "'");

  //! Offset into the volume data file if any.
  int offset = 0;  ospGeti(volume, "filename offset", &offset);  fseek(file, offset, SEEK_SET);

  //! Volume dimensions.
  osp::vec3i dimensions;  exitOnCondition(!ospGetVec3i(volume, "dimensions", &dimensions), "no volume dimensions specified");

  //! Voxel type string.
  char *voxelType;  exitOnCondition(!ospGetString(volume, "voxelType", &voxelType), "no voxel type specified");

  //! Supported voxel types.
  exitOnCondition(strcmp(voxelType, "float") && strcmp(voxelType, "uchar"), "unsupported voxel type");

  //! Voxel size in bytes.
  size_t voxelSize = strcmp(voxelType, "float") ? sizeof(unsigned char) : sizeof(float);

  //! Check if a subvolume of the volume has been specified.
  //! Subvolume parameters: subvolumeOffsets, subvolumeDimensions, subvolumeSteps.
  //! The subvolume defaults to full dimensions (allowing for just subsampling, for example).
  osp::vec3i subvolumeOffsets = osp::vec3i(0);  ospGetVec3i(volume, "subvolumeOffsets", &subvolumeOffsets);
  exitOnCondition(reduce_min(subvolumeOffsets) < 0 || reduce_max(subvolumeOffsets - dimensions) >= 0, "invalid subvolume offsets");

  osp::vec3i subvolumeDimensions = dimensions - subvolumeOffsets;  ospGetVec3i(volume, "subvolumeDimensions", &subvolumeDimensions);
  exitOnCondition(reduce_min(subvolumeDimensions) < 1 || reduce_max(subvolumeDimensions - (dimensions - subvolumeOffsets)) > 0, "invalid subvolume dimension(s) specified");

  osp::vec3i subvolumeSteps = osp::vec3i(1);  ospGetVec3i(volume, "subvolumeSteps", &subvolumeSteps);
  exitOnCondition(reduce_min(subvolumeSteps) < 1 || reduce_max(subvolumeSteps - (dimensions - subvolumeOffsets)) >= 0, "invalid subvolume steps");

  bool useSubvolume = false;

  //! The dimensions of the volume to be imported; this will be changed if a subvolume is specified.
  osp::vec3i importVolumeDimensions = dimensions;

  if(reduce_max(subvolumeOffsets) > 0 || subvolumeDimensions != dimensions || reduce_max(subvolumeSteps) > 1) {

    useSubvolume = true;

    //! The dimensions of the volume to be imported, considering the subvolume specified.
    importVolumeDimensions = osp::vec3i(subvolumeDimensions.x / subvolumeSteps.x + (subvolumeDimensions.x % subvolumeSteps.x != 0),
                                        subvolumeDimensions.y / subvolumeSteps.y + (subvolumeDimensions.y % subvolumeSteps.y != 0),
                                        subvolumeDimensions.z / subvolumeSteps.z + (subvolumeDimensions.z % subvolumeSteps.z != 0));

    //! Range check.
    exitOnCondition(reduce_min(importVolumeDimensions) <= 0, "invalid import volume dimensions");

    //! Update the provided dimensions of the volume for the subvolume specified.
    ospSetVec3i(volume, "dimensions", importVolumeDimensions);
  }

  //! Voxel count.
  size_t voxelCount = size_t(importVolumeDimensions.x) * importVolumeDimensions.y * importVolumeDimensions.z;

  //! Allocate memory for the voxel data.
  void *voxelData = new unsigned char[voxelCount * voxelSize];

  if(!useSubvolume) {

    //! Copy voxel data into the buffer.
    size_t voxelsRead = fread(voxelData, voxelSize, voxelCount, file);

    //! The end of the file may have been reached unexpectedly.
    exitOnCondition(voxelsRead != voxelCount, "end of volume file reached before read completed");

  } else {

    //! Allocate memory for a single row of voxel data.
    unsigned char *rowData = new unsigned char[dimensions.x * voxelSize];

    //! Allocate memory for a single row of voxel data for the subvolume.
    unsigned char *subvolumeRowData = new unsigned char[importVolumeDimensions.x * voxelSize];

    //! Read the subvolume data from the full volume.
    for(long i3=subvolumeOffsets.z; i3<subvolumeOffsets.z+subvolumeDimensions.z; i3+=subvolumeSteps.z) {

      for(long i2=subvolumeOffsets.y; i2<subvolumeOffsets.y+subvolumeDimensions.y; i2+=subvolumeSteps.y) {

        //! Seek to appropriate location in file.
        fseek(file, offset + i3*dimensions.y*dimensions.x*voxelSize + i2*dimensions.x*voxelSize, SEEK_SET);

        //! Read row from volume.
        size_t voxelsRead = fread(rowData, voxelSize, dimensions.x, file);

        //! The end of the file may have been reached unexpectedly.
        exitOnCondition(voxelsRead != dimensions.x, "end of volume file reached before read completed");

        //! Resample row for the subvolume.
        for(long i1=subvolumeOffsets.x; i1<subvolumeOffsets.x+subvolumeDimensions.x; i1+=subvolumeSteps.x)
          memcpy(&subvolumeRowData[(i1 - subvolumeOffsets.x) / subvolumeSteps.x * voxelSize], &rowData[i1 * voxelSize], voxelSize);

        //! Copy subvolume row into the buffer.
        size_t offset = (((i3 - subvolumeOffsets.z) / subvolumeSteps.z) * importVolumeDimensions.y*importVolumeDimensions.x + ((i2 - subvolumeOffsets.y) / subvolumeSteps.y) * importVolumeDimensions.x) * voxelSize;
        memcpy(&((unsigned char *)voxelData)[offset], &subvolumeRowData[0], importVolumeDimensions.x * voxelSize);
      }
    }

    //! Clean up.
    delete [] rowData;
  }

  //! Wrap the buffer in an OSPRay object using the OSP_DATA_SHARED_BUFFER flag to avoid copying the data.
  OSPData voxelDataObject = ospNewData(voxelCount, strcmp(voxelType, "float") ? OSP_UCHAR : OSP_FLOAT, voxelData, OSP_DATA_SHARED_BUFFER);

  //! Set the buffer as a parameter on the volume.
  ospSetData(volume, "voxelData", voxelDataObject);

  //! Return the volume.
  return(volume);

}

