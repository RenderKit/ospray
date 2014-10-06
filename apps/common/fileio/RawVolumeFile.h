//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

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

} // namespace ospray

