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

#pragma once

#include "OSPMPIConfig.h"
#include "volume/BlockBrickedVolume.h"

namespace ospray {

  /*! \brief A type of structured volume that is data-distributed
    across multiple clients */
  /*! \detailed This class implements a structured volume whose data
      is distributed across multiple different nodes. The overall data
      set is chopped into blocks (of a user-defined size) that are
      then replicated across the different clients in some sort of
      (typically round-robin) mode. The application can still call
      setRegion, in which case the region data gets sent to all
      clients, but gets only stored on those clients that actually own
      the respective voxels. */
  class DataDistributedBlockedVolume : public StructuredVolume
  {
  public:
    /*! a single data-distributed block */
    struct DDBlock
    {
      //! range of voxels in this block
      box3i domain;
      //! 3D bounding box
      box3f bounds;
      /*! ID of _first_ node that owns this block */
      int firstOwner;
      /*! number of nodes that own this block */
      int numOwners;
      /*! 'bool' of whether this block is owned by me, or not */
      int isMine;

      Ref<Volume> cppVolume;
      void   *ispcVolume;
    };

    //! Constructor.
    DataDistributedBlockedVolume();

    void updateEditableParameters() override;

    //! \brief Returns whether the volume is a data-distributed volume
    bool isDataDistributed() const override;

    void buildAccelerator() override;

    //! A string description of this class.
    std::string toString() const override;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    void commit() override;

    //! Copy voxels into the volume at the given index (non-zero return value
    //! indicates success).
    int setRegion(const void *source,
                  const vec3i &index,
                  const vec3i &count) override;

    //NOTE(jda) - a private section needs to be defined to make usage clearer
  //private:

    //! Create the equivalent ISPC volume container.
    void createEquivalentISPC() override;

    /*! size of each block, in voxels, WITHOUT padding (in practice the blocks
     *  WILL be padded) */
    vec3i blockSize;
    /*! number of blocks, per dimension */
    vec3i ddBlocks;
    /*! number of blocks total */
    size_t numDDBlocks;
    /*! list of data distributed blocks */
    DDBlock *ddBlock;
  };

} // ::ospray

