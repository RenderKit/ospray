//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include <algorithm>
#include <map>
#include <string>
#include "ospray/volume/StructuredVolumeFile.h"

namespace ospray {

    //! \brief A concrete implementation of the StructuredVolumeFile class
    //!  for reading voxel data stored in a file on disk as a single mono-
    //!  lithic brick.
    //!
    class RawVolumeFile : public StructuredVolumeFile {
    public:

        //! Constructor.
        RawVolumeFile(const std::string &filename) : filename(realpath(filename.c_str(), NULL)) { getKeyValueStrings(); }

        //! Destructor.
        virtual ~RawVolumeFile() {};

        //! Get the volume dimensions.
        virtual vec3i getVolumeDimensions();

        //! Copy data from the file into memory.
        virtual void getVoxelData(void **buffer);

        //! Copy data from the file into the volume.
        virtual void getVoxelData(StructuredVolume *volume);

        //! Get the voxel size in bytes.
        size_t getVoxelSizeInBytes();

        //! Get the voxel spacing.
        virtual vec3f getVoxelSpacing();

        //! Get the voxel type string.
        virtual std::string getVoxelType();

        //! A string description of this class.
        virtual std::string toString() const { return("ospray::RawVolumeFile<" + filename + ">"); }

    private:

        //! Path to the file containing the volume header.
        std::string filename;

        //! Key value pairs describing the volume.
        std::map<std::string, std::string> header;

        //! Error checking.
        void exitOnCondition(bool condition, const std::string &message) const { if (condition) throw std::runtime_error("OSPRay::RawVolumeFile error: " + message + "."); }

        //! Get key value pairs from a volume header file.
        void getKeyValueStrings();

        //! Locate and open the volume data file.
        FILE *openVoxelDataFile();

    };

} // namespace ospray

