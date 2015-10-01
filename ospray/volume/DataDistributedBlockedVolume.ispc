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

#include "ospray/volume/DataDistributedBlockedVolume.ih"

void DDBVolume_Constructor(DDBVolume *uniform self,
                           void *uniform cppEquivalent,
                           const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&self->super,cppEquivalent,dimensions);
  print("constructor missing\n");
}


/*! a dummy function, for now */
inline void DDBVolume_getVoxel(void *uniform _self,
                               const varying vec3i &index,
                               varying float &value)
{
  value = 0.5f;
}

export void *uniform DDBVolume_create(void *uniform cppEquivalent,
                                      uniform int voxelType,
                                      const uniform vec3i &dimensions,
                                      const uniform vec3i &ddBlocks,
                                      const uniform vec3i &blockSize,
                                      DDBVolumeBlock *uniform ddBlock
                                      )
{
  print("DDBVolume_create\n");
  DDBVolume *uniform self = uniform new uniform DDBVolume;
  DDBVolume_Constructor(self,cppEquivalent,dimensions);

  self->super.getVoxel = DDBVolume_getVoxel;

  return self;
}
