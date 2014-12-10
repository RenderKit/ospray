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

#pragma once

#include <string>
#include "ospray/common/Data.h"
#include "ospray/fileio/VolumeFile.h"
#include "ospray/volume/StructuredVolume.h"

namespace ospray {

  //! \brief A concrete implementation of the VolumeFile class for reading
  //!  for reading voxel data stored in a file on disk as a single mono-
  //!  lithic brick, where the volume specification is defined elsewhere.
  //!
  class RawVolumeFile : public VolumeFile {
  public:

    //! Constructor.
    RawVolumeFile(const std::string &filename) : filename(filename) {}

    //! Destructor.
    virtual ~RawVolumeFile() {};

    //! Import the volume data.
    virtual OSPObjectCatalog importVolume(Volume *volume);

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::RawVolumeFile"); }

  private:

    //! Path to the file containing the volume data.
    std::string filename;

    //! Copy a row of voxel data from the file into the volume.
    void importVoxelRow(FILE *file, StructuredVolume *volume, Data *buffer, const size_t &index);

    //! Locate and open the volume data file.
    FILE *openVolumeFile();

  };

} // ::ospray

