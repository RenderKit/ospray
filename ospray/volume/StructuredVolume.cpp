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
#include "ospray/common/data.h"
#include "ospray/common/library.h"
#include "ospray/volume/StructuredVolume.h"
#include "ospray/volume/StructuredVolumeFile.h"

namespace ospray {

    void StructuredVolume::commit() {

        //! For now we do not support changes to a StructuredVolume object after it has been committed.
        exitOnCondition(ispcEquivalent != NULL, "changes to volume objects are not allowed post-commit");

        //! The volume may be initialized from the contents of a file or from memory.
        const char *filename = getParamString("Filename", NULL);  (filename) ? getVolumeFromFile(filename) : getVolumeFromMemory();

    }

    OSPDataType StructuredVolume::getEnumForVoxelType() const {

        //! Separate out the base type and vector width.
        char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

        //! Unsigned character scalar and vector types.
        if (!strcmp(kind, "uchar")) return((width == 1) ? OSP_UCHAR : (width == 2) ? OSP_UCHAR2 : (width == 3) ? OSP_UCHAR3 : (width == 4) ? OSP_UCHAR4 : OSP_UNKNOWN);

        //! Floating point scalar and vector types.
        if (!strcmp(kind, "float")) return((width == 1) ? OSP_FLOAT : (width == 2) ? OSP_FLOAT2 : (width == 3) ? OSP_FLOAT3 : (width == 4) ? OSP_FLOAT4 : OSP_UNKNOWN);

        //! Unsigned integer scalar and vector types.
        if (!strcmp(kind, "uint")) return((width == 1) ? OSP_UINT : (width == 2) ? OSP_UINT2 : (width == 3) ? OSP_UINT3 : (width == 4) ? OSP_UINT4 : OSP_UNKNOWN);

        //! Signed integer scalar and vector types.
        if (!strcmp(kind, "int")) return((width == 1) ? OSP_INT : (width == 2) ? OSP_INT2 : (width == 3) ? OSP_INT3 : (width == 4) ? OSP_INT4 : OSP_UNKNOWN);

        //! Unknown type.
        return(OSP_UNKNOWN);

    }

    void StructuredVolume::getVolumeFromFile(const std::string &filename) {

        //! Create a concrete instance of a subclass of StructuredVolumeFile based on the file name extension.
        StructuredVolumeFile *volumeFile = StructuredVolumeFile::open(filename);  exitOnCondition(!volumeFile, "unrecognized volume file type '" + filename + "'");

        //! Set the volume dimensions.
        setDimensions(volumeFile->getVolumeDimensions());

        //! Set the transfer function.
        setTransferFunction((TransferFunction *) getParamObject("TransferFunction", NULL));

        //! Set the voxel spacing.
        setVoxelSpacing(volumeFile->getVoxelSpacing());

        //! Set the voxel type string.
        setVoxelType(volumeFile->getVoxelType());

        //! Create the equivalent ISPC volume container and allocate memory for voxel data.
        createEquivalentISPC();

        //! Copy voxel data into the volume.
        volumeFile->getVoxelData(this);

    }

    void StructuredVolume::getVolumeFromMemory() {

        //! Set the volume dimensions.
        setDimensions(getParam3i("Dimensions", vec3i(0)));  exitOnCondition(reduce_min(volumeDimensions) <= 0, "invalid volume dimensions");

        //! Set the transfer function.
        setTransferFunction((TransferFunction *) getParamObject("TransferFunction", NULL));

        //! Set the voxel spacing.
        setVoxelSpacing(getParam3f("VoxelSpacing", vec3f(1.0f)));

        //! Set the voxel type string.
        setVoxelType(getParamString("VoxelType", NULL));  exitOnCondition(voxelType.empty(), "no voxel type specified");

        //! Create the equivalent ISPC volume container and allocate memory for voxel data.
        createEquivalentISPC();

        //! Get a pointer to the source voxel data.
        const Data *voxelData = getParamData("VoxelData", NULL);  exitOnCondition(voxelData == NULL, "no voxel data was specified");

        //! The dimensions of the source voxel data and target volume must match.
        exitOnCondition(getVoxelCount() != voxelData->numItems, "unexpected dimensions of source voxel data");

        //! The source and target voxel types must match.
        exitOnCondition(getEnumForVoxelType() != voxelData->type, "unexpected source voxel type");

        //! Copy voxel data into the volume.
        setRegion(voxelData->data, vec3i(0, 0, 0), getDimensions());

    }

} // namespace ospray

