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
#include <cdds.h>
#include "ospray/volume/StructuredVolumeFile.h"

namespace ospray {

    //! \brief A concrete implementation of the StructuredVolumeFile class
    //!  for reading voxel data stored in seismic file formats on disk.
    //!
    class SeismicVolumeFile : public StructuredVolumeFile {
    public:

        //! Constructor.
        SeismicVolumeFile(const std::string &filename);

        //! Destructor.
        virtual ~SeismicVolumeFile() {};

        //! Get the volume dimensions.
        virtual vec3i getVolumeDimensions();

        //! Copy data from the file into memory.
        virtual void getVoxelData(void **buffer);

        //! Copy data from the file into the volume.
        virtual void getVoxelData(StructuredVolume *volume);

        //! Get the voxel spacing.
        virtual vec3f getVoxelSpacing();

        //! Get the voxel type string.
        virtual std::string getVoxelType();

        //! A string description of this class.
        virtual std::string toString() const { return("ospray::SeismicVolumeFile<" + filename + ">"); }

    private:

        //! Path to the file containing the volume header or the volume.
        std::string filename;

        //! Key value pairs from the header (if we have one) describing the volume.
        std::map<std::string, std::string> header;

        //! Seismic data attributes
        BIN_TAG inputBinTag;
        int traceHeaderSize;
        int origin[3];
        float deltas[3];
        int dimensions[3];

        //! Error checking.
        void exitOnCondition(bool condition, const std::string &message) const { if (condition) throw std::runtime_error("OSPRay::SeismicVolumeFile error: " + message + "."); }

        //! Read key value pairs from a volume header file.
        void readHeader();

        //! Open the seismic data file.
        bool openSeismicDataFile(std::string dataFilename);

        //! Scan the seismic data file to determine the volume dimensions.
        bool scanSeismicDataFileForDimensions();
    };

} // namespace ospray
