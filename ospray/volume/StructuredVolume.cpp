//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//
//    Authors: Gregory S. Johnson and Ingo Wald
//

#include <map>
#include "ospray/common/Data.h"
#include "ospray/common/Library.h"
#include "ospray/fileio/VolumeFile.h"
#include "ospray/volume/StructuredVolume.h"

namespace ospray {

    void StructuredVolume::commit() {

        //! For now we do not support changes to a StructuredVolume object after it has been committed.
        exitOnCondition(ispcEquivalent != NULL, "changes to volume objects are not allowed post-commit");

        //! Check for a file name.
        std::string filename = getParamString("filename", "");

        //! The volume may be initialized from the contents of a file or from memory.
        if (!filename.empty()) VolumeFile::importVolume(filename, this);  else getVolumeFromMemory();

    }

    OSPDataType StructuredVolume::getVoxelType() const {

        //! Separate out the base type and vector width.
        char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

        //! Single precision scalar floating point.
        if (!strcmp(kind, "float") && width == 1) return(OSP_FLOAT);

        //! Unsigned 8-bit scalar integer.
        if (!strcmp(kind, "uchar") && width == 1) return(OSP_UCHAR);

        //! Unknown type.
        return(OSP_UNKNOWN);

    }

    void StructuredVolume::getVolumeFromMemory() {

        //! Create the equivalent ISPC volume container and allocate memory for voxel data.
        createEquivalentISPC();

        //! Get a pointer to the source voxel data.
        const Data *voxelData = getParamData("voxelData", NULL);  exitOnCondition(voxelData == NULL, "no voxel data was specified");

        //! The dimensions of the source voxel data and target volume must match.
        exitOnCondition(volumeDimensions.x * volumeDimensions.y * volumeDimensions.z != voxelData->numItems, "unexpected dimensions of source voxel data");

        //! The source and target voxel types must match.
        exitOnCondition(getVoxelType() != voxelData->type, "unexpected source voxel type");

        //! Copy voxel data into the volume.
        setRegion(voxelData->data, vec3i(0, 0, 0), volumeDimensions);

    }

    size_t StructuredVolume::getVoxelSizeInBytes() const {

        //! Separate out the base type and vector width.
        char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

        //! Unsigned character scalar and vector types.
        if (!strcmp(kind, "uchar") && width >= 1 && width <= 4) return(sizeof(unsigned char) * width);

        //! Floating point scalar and vector types.
        if (!strcmp(kind, "float") && width >= 1 && width <= 4) return(sizeof(float) * width);

        //! Unsigned integer scalar and vector types.
        if (!strcmp(kind, "uint") && width >= 1 && width <= 4) return(sizeof(unsigned int) * width);

        //! Signed integer scalar and vector types.
        if (!strcmp(kind, "int") && width >= 1 && width <= 4) return(sizeof(int) * width);  return(0);

    }

} // namespace ospray

