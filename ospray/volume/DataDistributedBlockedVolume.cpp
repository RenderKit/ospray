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

#include <limits>
// ospray
#include "ospray/volume/DataDistributedBlockedVolume.h"
#include "ospray/common/Core.h"
#include "ospray/transferFunction/TransferFunction.h"
#if EXP_DATA_PARALLEL
#include "ospray/mpi/MPICommon.h"
#endif
// ispc exports:
#include "DataDistributedBlockedVolume_ispc.h"

namespace ospray {

#if EXP_DATA_PARALLEL

  //! Allocate storage and populate the volume, called through the OSPRay API.
  void DataDistributedBlockedVolume::commit()
  {
    // IW: do NOT call parent commit here - this would try to build
    // the parent acceleration strucutre, which doesn't make sense on
    // blocks that do not exist!!!!

    updateEditableParameters();//
    
    StructuredVolume::commit();

    for (int i=0;i<numDDBlocks;i++) {
      DDBlock *block = ddBlock+i;
      if (!block->isMine) {
        continue;
      }
      block->cppVolume->commit();
    }
  }

  DataDistributedBlockedVolume::DataDistributedBlockedVolume() :
    numDDBlocks(0),
    ddBlock(NULL)
  {
    PING;
  }

  void DataDistributedBlockedVolume::updateEditableParameters()
  {
    StructuredVolume::updateEditableParameters();
    for (int i=0;i<numDDBlocks;i++) {
      DDBlock *block = ddBlock+i;

      if (!block->isMine) {
        continue;
      }

      Ref<TransferFunction> transferFunction
        = (TransferFunction *)getParamObject("transferFunction", NULL);

      auto &cppVolume = block->cppVolume;
      cppVolume->findParam("transferFunction",1)->set(transferFunction.ptr);

      cppVolume->findParam("samplingRate",1)->set(getParam1f("samplingRate",
                                                             1.f));
      cppVolume->findParam("gradientShadingEnabled",
                           1)->set(getParam1i("gradientShadingEnabled", 0));

      cppVolume.ptr->updateEditableParameters();
    }
  }

  bool DataDistributedBlockedVolume::isDataDistributed() const
  {
    return true;
  }

  void DataDistributedBlockedVolume::buildAccelerator()
  {
    std::cout << "intentionally SKIP building an accelerator for data parallel "
              << "volume (this'll be done on the brick level)" << std::endl;
  }

  std::string DataDistributedBlockedVolume::toString() const
  {
    return("ospray::DataDistributedBlockedVolume<" + voxelType + ">");
  }
  
  //! Copy voxels into the volume at the given index (non-zero return value
  //! indicates success).
  int DataDistributedBlockedVolume::setRegion(
      /* points to the first voxel to be copies. The
         voxels at 'soruce' MUST have dimensions
         'regionSize', must be organized in 3D-array
         order, and must have the same voxel type as the
         volume.*/
      const void *source,
      /*! coordinates of the lower,
        left, front corner of the target
        region.*/
      const vec3i &regionCoords,
      /*! size of the region that we're writing to; MUST
        be the same as the dimensions of source[][][] */
      const vec3i &regionSize)
  {
    // Create the equivalent ISPC volume container and allocate memory for voxel
    // data.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    for (int i=0;i<numDDBlocks;i++) {
      // that block isn't mine, I shouldn't care ...
      if (!ddBlock[i].isMine) continue;
      
      // first, do some culling to make sure we only do setrgion
      // calls on blocks that actually map to this block
      if (ddBlock[i].domain.lower.x >= regionCoords.x+regionSize.x) continue;
      if (ddBlock[i].domain.lower.y >= regionCoords.y+regionSize.y) continue;
      if (ddBlock[i].domain.lower.z >= regionCoords.z+regionSize.z) continue;
      
      if (ddBlock[i].domain.upper.x+regionSize.x < regionCoords.x) continue;
      if (ddBlock[i].domain.upper.y+regionSize.y < regionCoords.y) continue;
      if (ddBlock[i].domain.upper.z+regionSize.z < regionCoords.z) continue;
      
      ddBlock[i].cppVolume->setRegion(source,
                                      regionCoords-ddBlock[i].domain.lower,
                                      regionSize);

      ddBlock[i].ispcVolume = ddBlock[i].cppVolume->getIE();

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
      ManagedObject::Param *param = ddBlock[i].cppVolume->findParam("voxelRange");
      if (param != NULL && param->type == OSP_FLOAT2){
        vec2f blockRange = ((vec2f*)param->f)[0];
        voxelRange.x = std::min(voxelRange.x, blockRange.x);
        voxelRange.y = std::max(voxelRange.y, blockRange.y);
      }
#endif
    }

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    // Do a reduction here to worker 0 since it will be queried for the voxel range by the display node
    vec2f globalVoxelRange = voxelRange;
    MPI_CALL(Reduce(&voxelRange.x, &globalVoxelRange.x, 1, MPI_FLOAT, MPI_MIN, 0, mpi::worker.comm));
    MPI_CALL(Reduce(&voxelRange.y, &globalVoxelRange.y, 1, MPI_FLOAT, MPI_MAX, 0, mpi::worker.comm));
    set("voxelRange", globalVoxelRange);
#endif
    return 0;
  }

  void DataDistributedBlockedVolume::createEquivalentISPC()
  {
    if (ispcEquivalent != NULL) return;

    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, 
                    "unrecognized voxel type (must be set before calling "
                    "ospSetRegion())");
    
    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions (must be set before calling "
                    "ospSetRegion())");

    ddBlocks    = getParam3i("num_dp_blocks",vec3i(4,4,4));
    blockSize   = divRoundUp(dimensions,ddBlocks);
    std::cout << "#osp:dp: using data parallel volume of " << ddBlocks
              << " blocks, blockSize is " << blockSize << std::endl;
    
    // Set the grid origin, default to (0,0,0).
    this->gridOrigin = getParam3f("gridOrigin", vec3f(0.f));

    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions");

    // Set the grid spacing, default to (1,1,1).
    this->gridSpacing = getParam3f("gridSpacing", vec3f(1.f));

    numDDBlocks = ospcommon::reduce_mul(ddBlocks);
    ddBlock     = new DDBlock[numDDBlocks];

    printf("=======================================================\n");
    printf("created %ix%ix%i data distributed volume blocks\n",
           ddBlocks.x,ddBlocks.y,ddBlocks.z);
    printf("=======================================================\n");

    if (!ospray::core::isMpiParallel()) {
      throw std::runtime_error("data parallel volume, but not in mpi parallel "
                               "mode...");
    }
    int64 numWorkers = ospray::core::getWorkerCount();
    // PRINT(numDDBlocks);
    // PRINT(numWorkers);

    voxelType = getParamString("voxelType", "unspecified");  

    // if (numDDBlocks >= numWorkers) {
    // we have more workers than blocks - use one owner per block,
    // meaning we'll end up with multiple blocks per worker
    int blockID = 0;
    for (int iz=0;iz<ddBlocks.z;iz++)
      for (int iy=0;iy<ddBlocks.y;iy++)
        for (int ix=0;ix<ddBlocks.x;ix++, blockID++) {
          DDBlock *block = &ddBlock[blockID];
          if (numDDBlocks >= numWorkers) {
            block->firstOwner = blockID % numWorkers;
            block->numOwners = 1;
          } else {
            block->firstOwner = (blockID * numWorkers) / numDDBlocks;
            int nextBlockFirstOwner = ((blockID+1)*numWorkers) / numDDBlocks;
            block->numOwners = nextBlockFirstOwner - block->firstOwner; // + 1;
          }
          block->isMine 
            = (ospray::core::getWorkerRank() >= block->firstOwner)
            && (ospray::core::getWorkerRank() <
                (block->firstOwner + block->numOwners));
          block->domain.lower = vec3i(ix,iy,iz) * blockSize;
          block->domain.upper = min(block->domain.lower+blockSize,dimensions);
          block->bounds.lower = gridOrigin +
            (gridSpacing * vec3f(block->domain.lower));
          block->bounds.upper = gridOrigin +
            (gridSpacing * vec3f(block->domain.upper));
          
          // iw: add one voxel overlap, so we can properly interpolate
          // even across block boundaries (we need the full *cell* on
          // the right side, not just the last *voxel*!)
          block->domain.upper = min(block->domain.upper+vec3i(1),dimensions);
          
          
          if (block->isMine) {
            Ref<Volume> volume = new BlockBrickedVolume;
            vec3i blockDims = block->domain.upper - block->domain.lower;
            volume->findParam("dimensions",1)->set(blockDims);
            volume->findParam("gridOrigin",1)->set(block->bounds.lower);
            volume->findParam("gridSpacing",1)->set(gridSpacing);
            volume->findParam("voxelType",1)->set(voxelType.c_str());
            
            printf("rank %li owns block %i,%i,%i (ID %i), dims %i %i %i\n",
                   (size_t)core::getWorkerRank(),ix,iy,iz,
                   blockID,blockDims.x,blockDims.y,blockDims.z);
            block->cppVolume = volume;
            block->ispcVolume = NULL; //volume->getIE();
          } else {
            block->ispcVolume = NULL;
            block->cppVolume = NULL;
          }
        }
    
    // Create an ISPC BlockBrickedVolume object and assign type-specific
    // function pointers.
    ispcEquivalent = ispc::DDBVolume_create(this,                                                             
                                            (int)getVoxelType(), 
                                            (const ispc::vec3i&)dimensions,
                                            (const ispc::vec3i&)ddBlocks,
                                            (const ispc::vec3i&)blockSize,
                                            ddBlock);
    if (!ispcEquivalent) {
      throw std::runtime_error("could not create create data distributed "
                               "volume");
    }
  }

  // A volume type with internal data-distribution. needs a renderer
  // that is capable of data-parallel rendering!
  OSP_REGISTER_VOLUME(DataDistributedBlockedVolume, data_distributed_volume);

#endif

} // ::ospray
