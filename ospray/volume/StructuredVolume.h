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

    //! \brief A StructuredVolume is an abstraction for uniform volume
    //!  subtypes.
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
        StructuredVolume() : volumeDimensions(0), voxelType("undefined") {}

        //! Destructor.
        virtual ~StructuredVolume() {};

        //! Allocate storage and populate the volume.
        virtual void commit();

        //! Get the volume dimensions.
        const vec3i &getDimensions() const { return(volumeDimensions); }

        //! Get the OSPDataType enum corresponding to the voxel type string.
        OSPDataType getEnumForVoxelType() const;

        //! Get the volume size in voxels.
        size_t getVoxelCount() const { return(volumeDimensions.x * volumeDimensions.y * volumeDimensions.z); }

        //! Get the voxel spacing.
        vec3f getVoxelSpacing() const { return(voxelSpacing); }

        //! Get the voxel type string.
        std::string getVoxelType() const { return(voxelType); }

        //! Set the volume dimensions.
        void setDimensions(const vec3i &size) { volumeDimensions = size; }

        //! Copy voxels into the volume at the given index.
        virtual void setRegion(const void *source, const vec3i &index, const vec3i &count) = 0;

        //! Set the voxel spacing.
        void setVoxelSpacing(const vec3f &spacing) { voxelSpacing = spacing; }

        //! Set the voxel type string.
        void setVoxelType(const std::string &type) { voxelType = type; }

        //! A string description of this class.
        virtual std::string toString() const { return("ospray::StructuredVolume<" + voxelType + ">"); }

    protected:

        //! Volume size in voxels per dimension.
        vec3i volumeDimensions;

        //! Voxel spacing.
        vec3f voxelSpacing;

        //! Voxel type.
        std::string voxelType;

        //! Create the equivalent ISPC volume container.
        virtual void createEquivalentISPC() = 0;

        //! Error checking.
        virtual void exitOnCondition(bool condition, const std::string &message) const { if (condition) throw std::runtime_error("OSPRay::StructuredVolume error: " + message + "."); }

        //! Initialize the volume from the contents of a file.
        void getVolumeFromFile(const std::string &filename);

        //! Initialize the volume from memory.
        void getVolumeFromMemory();

    };

} // namespace ospray

