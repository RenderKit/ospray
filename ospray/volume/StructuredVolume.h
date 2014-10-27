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
#include "ospray/volume/Volume.h"

namespace ospray {

    //! \brief A StructuredVolume is an abstraction for Volume subtypes
    //!  in which the voxels are implicitly ordered.
    //!
    //!  The actual memory layout, dimensionality, and source of samples
    //!  are unknown to this class.  Subclasses may implement specific
    //!  memory layouts, addressing precision, and voxel types.  A type
    //!  string passed to Volume::createInstance() specifies a particular
    //!  concrete implementation.  This type string must be registered
    //!  in OSPRay proper, or in a loaded module via OSP_REGISTER_VOLUME.
    //!
    class StructuredVolume : public Volume {
    public:

        //! Constructor.
        StructuredVolume() {}

        //! Destructor.
        virtual ~StructuredVolume() {};

        //! Allocate storage and populate the volume, called through the OSPRay API.
        virtual void commit();

        //! Create the equivalent ISPC volume container.
        virtual void createEquivalentISPC() = 0;

        //! Volume size in voxels per dimension.
        const vec3i &getDimensions() const { return(volumeDimensions); }

        //! Voxel size in bytes.
        size_t getVoxelSizeInBytes() const;

        //! Get the OSPDataType enum corresponding to the voxel type string.
        OSPDataType getVoxelType() const;

        //! Copy voxels into the volume at the given index.
        virtual void setRegion(const void *source, const vec3i &index, const vec3i &count) = 0;

        //! A string description of this class.
        virtual std::string toString() const { return("ospray::StructuredVolume<" + voxelType + ">"); }

    protected:

        //! Volume size in voxels per dimension.
        vec3i volumeDimensions;

        //! Voxel type.
        std::string voxelType;

        //! Initialize the volume from memory.
        void getVolumeFromMemory();

        //! Update select parameters after the volume has been allocated and filled.
        virtual void updateEditableParameters() { return; }

    };

} // namespace ospray

