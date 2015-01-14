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

  //! Voxel count.
  size_t voxelCount = dimensions.x * dimensions.y * dimensions.z;

  //! Allocate memory for the voxel data.
  void *voxelData = new unsigned char[voxelCount * voxelSize];

  //! Copy voxel data into the buffer.
  size_t voxelsRead = fread(voxelData, voxelSize, voxelCount, file);

  //! The end of the file may have been reached unexpectedly.
  exitOnCondition(voxelsRead != voxelCount, "end of volume file reached before read completed");

  //! Wrap the buffer in an OSPRay object using the OSP_DATA_SHARED_BUFFER flag to avoid copying the data.
  OSPData voxelDataObject = ospNewData(voxelCount, strcmp(voxelType, "float") ? OSP_UCHAR : OSP_FLOAT, voxelData, OSP_DATA_SHARED_BUFFER);

  //! Set the buffer as a parameter on the volume.
  ospSetData(volume, "voxelData", voxelDataObject);  return(volume);

}

