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

// ospray
#include "ospray/volume/DataDistributedBlockedVolume.h"
#include "ospray/common/Core.h"
#include "ospray/transferFunction/TransferFunction.h"
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

    // StructuredVolume::commit();


    StructuredVolume::commit();
    for (int i=0;i<numDDBlocks;i++) {
      DDBlock *block = ddBlock+i;
      if (!block->isMine) continue;
      block->cppVolume->commit();
      // PRINT(((BlockBrickedVolume*)block->cppVolume)->voxelRange);
      // std::cout << "warning - voxelRange not properly set for this type (because the 'setregion's also include _all_ voxels, even from those outside the block!) ... " << std::endl;
    }
  }

  void DataDistributedBlockedVolume::updateEditableParameters()
  {
    StructuredVolume::updateEditableParameters();
    for (int i=0;i<numDDBlocks;i++) {
      DDBlock *block = ddBlock+i;
      if (!block->isMine) continue;

      Ref<TransferFunction> transferFunction
        = (TransferFunction *)getParamObject("transferFunction", NULL);
      block->cppVolume->findParam("transferFunction",1)->set(transferFunction.ptr);
    }
  }
  
  //! Copy voxels into the volume at the given index (non-zero return value indicates success).
  int DataDistributedBlockedVolume::setRegion(/* points to the first voxel to be copies. The
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
    // PING;
    // PRINT(regionCoords);
    // PRINT(regionSize);
    // Create the equivalent ISPC volume container and allocate memory for voxel data.
    if (ispcEquivalent == NULL) createEquivalentISPC();
    
    for (int i=0;i<numDDBlocks;i++) {
      if (ddBlock[i].isMine) {
        // std::cout << "setting region in block " << i << std::endl;
        ddBlock[i].cppVolume->setRegion(source,regionCoords-ddBlock[i].domain.lower,
                                        regionSize);
        ddBlock[i].ispcVolume = ddBlock[i].cppVolume->getIE();
      }
    }
    // float f = 0.5f;
    // computeVoxelRange(&f,1);

    return 0;
  }

  void DataDistributedBlockedVolume::createEquivalentISPC()
  {
    if (ispcEquivalent != NULL) return;

    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, 
                    "unrecognized voxel type (must be set before calling ospSetRegion())");
    
    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions (must be set before calling ospSetRegion())");


    // Set the grid origin, default to (0,0,0).
    this->gridOrigin = getParam3f("gridOrigin", vec3f(0.f));

    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions");

    // Set the grid spacing, default to (1,1,1).
    this->gridSpacing = getParam3f("gridSpacing", vec3f(1.f));

    // =======================================================
    // by default, create 2x2x2 blocks
    // =======================================================
    ddBlocks = vec3i(2);
    const char *ddBlockStringFromEnv = getenv("OSPRAY_DD_NUM_BLOCKS");
    if (ddBlockStringFromEnv) {
      std::cout << "#osp: getting num DD blocks from env string OSPRAY_DD_NUM_BLOCKS='"
                << ddBlockStringFromEnv << "'" << std::endl;
      int numParsed = sscanf(ddBlockStringFromEnv,"%dx%dx%d",
                             &ddBlocks.x,&ddBlocks.y,&ddBlocks.z);
      if (numParsed != 3)
        throw std::runtime_error("#osp: cannot parse volume dimensions string "
                                 "(has to be XxYxZ)");
    }

    blockSize = divRoundUp(dimensions,ddBlocks);

    numDDBlocks = embree::reduce_mul(ddBlocks);
    ddBlock = new DDBlock[numDDBlocks];

    printf("=======================================================\n");
    printf("created %ix%ix%i data distributed volume blocks\n",
           ddBlocks.x,ddBlocks.y,ddBlocks.z);
    printf("=======================================================\n");

    if (!ospray::core::isMpiParallel)
      throw std::runtime_error("data parallel volume, but not in mpi parallel mode...");
    int64 numWorkers = ospray::core::getWorkerCount();
    PRINT(numDDBlocks);
    PRINT(numWorkers);

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
              // block->isMine  = block->firstOwner==ospray::core::getWorkerRank();
            } else {
              block->firstOwner = (blockID * numWorkers) / numDDBlocks;
              int nextBlockFirstOwner = ((blockID+1) * numWorkers) / numDDBlocks;
              block->numOwners = nextBlockFirstOwner - block->firstOwner;
            }
            block->isMine 
              = ((ospray::core::getWorkerRank()-block->firstOwner) < block->numOwners);
            block->domain.lower = vec3i(ix,iy,iz) * blockSize;
            block->domain.upper = min(block->domain.lower+blockSize,dimensions);
            block->bounds.lower = gridOrigin + gridSpacing * vec3f(block->domain.lower);// / vec3f(dimensions);
            block->bounds.upper = gridOrigin + gridSpacing * vec3f(block->domain.upper);// / vec3f(dimensions);

            block->domain.upper = min(block->domain.upper+vec3i(1),dimensions);

            
            if (block->isMine) {
              Ref<Volume> volume = new BlockBrickedVolume;
              vec3i blockDims = block->domain.upper-block->domain.lower;
              volume->findParam("dimensions",1)->set(blockDims);
              volume->findParam("gridOrigin",1)->set(block->bounds.lower);
              volume->findParam("gridSpacing",1)->set(gridSpacing);
              volume->findParam("voxelType",1)->set(voxelType.c_str());

              block->cppVolume = volume;
              block->ispcVolume = NULL; //volume->getIE();
            } else {
              block->ispcVolume = NULL;
              block->cppVolume = NULL;
            }
          }
    // } else {
    //   FATAL("not implemented yet - more workers than blocks ...");//TODO
    // }

    // Create an ISPC BlockBrickedVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::DDBVolume_create(this,                                                             
                                            (int)getVoxelType(), 
                                            (const ispc::vec3i&)dimensions,
                                            (const ispc::vec3i&)ddBlocks,
                                            (const ispc::vec3i&)blockSize,
                                            ddBlock);
    if (!ispcEquivalent) 
      throw std::runtime_error("could not create create data distributed volume");
  }

  // A volume type with internal data-distribution. needs a renderer
  // that is capable of data-parallel rendering!
  OSP_REGISTER_VOLUME(DataDistributedBlockedVolume, data_distributed_volume);

#endif

} // ::ospray


