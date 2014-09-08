//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <cassert>
#include "ospray/volume/BrickedVolume.h"
#include "BrickedVolumeFloat_ispc.h"
#include "BrickedVolumeUChar_ispc.h"

namespace ospray {

    bool inRange(const vec3i &value, const vec3i &lower, const vec3i &upper) {

        //! The lower bound is inclusive.
        if (value.x < lower.x || value.y < lower.y || value.z < lower.z) return(false);

        //! The upper bound is exclusive.
        if (value.x >= upper.x || value.y >= upper.y || value.z >= upper.z) return(false);

        //! The vector is within the given range only if the individual components are in range.
        return(true);

    }

    void BrickedVolume::createEquivalentISPC() {

        //! Allocate storage for single precision floating point voxel data.
        if (voxelType == "float") ispcEquivalent = ispc::BrickedVolumeFloat_createInstance((ispc::vec3i &) getDimensions(), transferFunction->getIE());

        //! Allocate storage for unsigned 8-bit integer voxel data.
        if (voxelType == "uchar") ispcEquivalent = ispc::BrickedVolumeUChar_createInstance((ispc::vec3i &) getDimensions(), transferFunction->getIE());

        //! The object may not have been created if out of memory or voxel type is unknown.
        exitOnCondition(ispcEquivalent == NULL, "unable to allocate storage for volume, out of memory or unknown voxel type");

    }

    void BrickedVolume::setRegion(const void *source, const vec3i &index, const vec3i &count) {

        //! Range check.
        assert(inRange(index, vec3i(0), getDimensions()) && inRange(count, vec3i(1), getDimensions() + 1));

        //! Copy single precision floating point voxel data into the volume.
        if (voxelType == "float") ispc::BrickedVolumeFloat_setRegion(getEquivalentISPC(), (const float *) source, (const ispc::vec3i &) index, (const ispc::vec3i &) count);

        //! Copy unsigned 8-bit integer voxel data into the volume.
        if (voxelType == "uchar") ispc::BrickedVolumeUChar_setRegion(getEquivalentISPC(), (const uint8 *) source, (const ispc::vec3i &) index, (const ispc::vec3i &) count);

    }

} // namespace ospray

