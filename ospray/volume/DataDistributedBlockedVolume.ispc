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

#include "volume/DataDistributedBlockedVolume.ih"

void DDBVolume_Constructor(DDBVolume *uniform self,
                           void *uniform cppEquivalent,
                           const uniform vec3i &dimensions)
{
  StructuredVolume_Constructor(&self->super,cppEquivalent,dimensions);
}


/*! a dummy function, for now */
inline void DDBVolume_getVoxel(void *uniform _self,
                               const varying vec3i &index,
                               varying float &value)
{
  print("#osp:dp: DDBVolume_getVoxel: should never actually get called - the data parallel volume is just a proxy for the data parallel blocks for now\n");
assert(0);
}

export void *uniform DDBVolume_create(void *uniform cppEquivalent,
                                      uniform int voxelType,
                                      const uniform vec3i &dimensions,
                                      const uniform vec3i &ddBlocks,
                                      const uniform vec3i &blockSize,
                                      void *uniform _ddBlock
                                      )
{
  DDBVolume *uniform self = uniform new uniform DDBVolume;
  DDBVolume_Constructor(self,cppEquivalent,dimensions);

  DDBVolumeBlock *uniform ddBlock = (DDBVolumeBlock *uniform)_ddBlock;

  self->super.getVoxel = NULL; //DDBVolume_getVoxel;
  self->super.super.computeSample = NULL; //DDBVolume_getVoxel;

  return self;
}
