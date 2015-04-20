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

inline void SharedStructuredVolumeFloat_getVoxel(void *uniform _volume, const varying vec3i &index, varying float &value)
{
  //! Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  const float *uniform voxelData = (const float *uniform) volume->voxelData;

  //! The voxel value at the given index.
  value = voxelData[index.x + volume->inherited.dimensions.x * (index.y + volume->inherited.dimensions.y * index.z)];
}

inline void SharedStructuredVolumeUChar_getVoxel(void *uniform _volume, const varying vec3i &index, varying float &value)
{
  //! Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume = (SharedStructuredVolume *uniform) _volume;

  //! Cast to the actual voxel type.
  const uint8 *uniform voxelData = (const uint8 *uniform) volume->voxelData;

  //! The voxel value at the given index.
  value = voxelData[index.x + volume->inherited.dimensions.x * (index.y + volume->inherited.dimensions.y * index.z)];
}

void SharedStructuredVolume_Constructor(SharedStructuredVolume *uniform volume, const uniform int voxelType, const uniform vec3i &dimensions, const void *uniform voxelData)
{
  StructuredVolume_Constructor(&volume->inherited, dimensions);

  volume->voxelData = voxelData;
  volume->voxelType = (OSPDataType) voxelType;
  volume->inherited.getVoxel = (volume->voxelType == OSP_FLOAT) ? SharedStructuredVolumeFloat_getVoxel : SharedStructuredVolumeUChar_getVoxel;
}

export void *uniform SharedStructuredVolume_createInstance(const uniform int voxelType, const uniform vec3i &dimensions, const void *uniform voxelData)
{
  //! The volume container.
  SharedStructuredVolume *uniform volume = uniform new uniform SharedStructuredVolume;

  SharedStructuredVolume_Constructor(volume, voxelType, dimensions, voxelData);

  return volume;
}
