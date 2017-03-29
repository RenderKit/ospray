// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// ours
#include "DataDistributedBlockedVolume.h"

// ospray
#include "ospray/volume/GhostBlockBrickedVolume.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "ospray/volume/GhostBlockBrickedVolume.h"

// comps
#include "components/mpiCommon/MPICommon.h"

// ispc exports:
#include "DataDistributedBlockedVolume_ispc.h"

namespace ospray {

  //! Allocate storage and populate the volume, called through the OSPRay API.
  void DataDistributedBlockedVolume::commit()
  {
    // IW: do NOT call parent commit here - this would try to build
    // the parent acceleration strucutre, which doesn't make sense on
    // blocks that do not exist!!!!

    updateEditableParameters();//
    
    StructuredVolume::commit();

    for (uint32_t i=0;i<numDDBlocks;i++) {
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
    for (uint32_t i = 0; i < numDDBlocks; i++) {
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

    // The block domains are in terms of the scaled regions so we must check
    // if the data is for the block in the scaled volume
    vec3i finalRegionCoords(vec3f(regionCoords) * scaleFactor);
    vec3i finalRegionSize(vec3f(regionSize) * scaleFactor);
    if (scaleFactor != vec3f(-1.f)) {
      finalRegionCoords = vec3i(vec3f(regionCoords) * scaleFactor);
      finalRegionSize = vec3i(vec3f(regionSize) * scaleFactor);
    } else {
      finalRegionCoords = regionCoords;
      finalRegionSize = regionSize;
    }

    for (uint32_t i = 0; i < numDDBlocks; i++) {
      // that block isn't mine, I shouldn't care ...
      if (!ddBlock[i].isMine) continue;
      
      // first, do some culling to make sure we only do setrgion
      // calls on blocks that actually map to this block
      if (ddBlock[i].domain.lower.x >= finalRegionCoords.x+finalRegionSize.x) continue;
      if (ddBlock[i].domain.lower.y >= finalRegionCoords.y+finalRegionSize.y) continue;
      if (ddBlock[i].domain.lower.z >= finalRegionCoords.z+finalRegionSize.z) continue;
      
      if (ddBlock[i].domain.upper.x+finalRegionSize.x < finalRegionCoords.x) continue;
      if (ddBlock[i].domain.upper.y+finalRegionSize.y < finalRegionCoords.y) continue;
      if (ddBlock[i].domain.upper.z+finalRegionSize.z < finalRegionCoords.z) continue;

      vec3i setRegionCoords = finalRegionCoords - ddBlock[i].domain.lower;
      if (scaleFactor != vec3f(-1.f)) {
        setRegionCoords = vec3i(vec3f(setRegionCoords) / scaleFactor);
      }
      
      ddBlock[i].cppVolume->setRegion(source, setRegionCoords, regionSize);

      ddBlock[i].ispcVolume = ddBlock[i].cppVolume->getIE();

    }

    return true;
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

    this->scaleFactor = getParam3f("scaleFactor", vec3f(-1.f));

    numDDBlocks = ospcommon::reduce_mul(ddBlocks);
    ddBlock     = new DDBlock[numDDBlocks];

    printf("=======================================================\n");
    printf("created %ix%ix%i data distributed volume blocks\n",
           ddBlocks.x,ddBlocks.y,ddBlocks.z);
    printf("=======================================================\n");

    if (!ospray::mpi::isMpiParallel()) {
      throw std::runtime_error("data parallel volume, but not in mpi parallel "
                               "mode...");
    }
    uint64_t numWorkers = mpi::getWorkerCount();
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
            = (ospray::mpi::getWorkerRank() >= block->firstOwner)
            && (ospray::mpi::getWorkerRank() <
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
#ifdef EXP_NEW_BB_VOLUME_KERNELS
            Ref<Volume> volume = new GhostBlockBrickedVolume;
#else
            Ref<Volume> volume = new BlockBrickedVolume;
#endif
            vec3i blockDims = block->domain.upper - block->domain.lower;
            volume->findParam("dimensions",1)->set(blockDims);
            volume->findParam("gridOrigin",1)->set(block->bounds.lower);
            volume->findParam("gridSpacing",1)->set(gridSpacing);
            volume->findParam("voxelType",1)->set(voxelType.c_str());
            if (scaleFactor != vec3f(-1.f)) {
              volume->findParam("scaleFactor",1)->set(scaleFactor);
            }
            
            printf("worker rank %li owns block %i,%i,%i (ID %i), dims %i %i %i\n",
                   (size_t)mpi::getWorkerRank(),ix,iy,iz,
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

} // ::ospray
