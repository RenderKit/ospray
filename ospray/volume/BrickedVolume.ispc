//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/volume/BrickedVolume.ih"

#define BRICK_VOXEL_WIDTH_BITCOUNT  (4)
#define BRICK_VOXEL_WIDTH           (1 << BRICK_VOXEL_WIDTH_BITCOUNT)
#define BRICK_VOXEL_COUNT           (BRICK_VOXEL_WIDTH * BRICK_VOXEL_WIDTH * BRICK_VOXEL_WIDTH)

varying vec3f BrickedVolume_computeGradient(void *uniform pointer, const varying vec3f &coordinates) {

    //! Cast to the actual Volume type.
    Volume *uniform volume = (Volume *uniform) pointer;

    //! The gradient will be computed using central differences.
    varying vec3f gradient;

    //! Gradient magnitude in the X direction.
    gradient.x = volume->computeSample(volume, coordinates + make_vec3f(1.0f, 0.0f, 0.0f)) - volume->computeSample(volume, coordinates + make_vec3f(-1.0f, 0.0f, 0.0f));

    //! Gradient magnitude in the Y direction.
    gradient.y = volume->computeSample(volume, coordinates + make_vec3f(0.0f, 1.0f, 0.0f)) - volume->computeSample(volume, coordinates + make_vec3f(0.0f, -1.0f, 0.0f));

    //! Gradient magnitude in the Z direction.
    gradient.z = volume->computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f, 1.0f)) - volume->computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f, -1.0f));

    //! This approximation may yield image artifacts.
    return(gradient * 0.5f);

}

varying float BrickedVolume_computeSample(void *uniform pointer, const varying vec3f &coordinates) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Coordinates outside the volume are clamped to the volume bounds.
    const vec3f clampedCoordinates = clamp(coordinates, make_vec3f(0.0f), volume->inherited.upperBound);

    //! Lower and upper corners of the box straddling the voxels to be interpolated.
    const vec3i voxelIndex_0 = integer_cast(clampedCoordinates);  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

    //! Fractional coordinates within the lower corner voxel used during interpolation.
    const vec3f fractionalCoordinates = clampedCoordinates - float_cast(voxelIndex_0);

    //! Look up the voxel values to be interpolated.
    const float voxelValue_000 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
    const float voxelValue_001 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
    const float voxelValue_010 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
    const float voxelValue_011 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
    const float voxelValue_100 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
    const float voxelValue_101 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
    const float voxelValue_110 = volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
    const float voxelValue_111 = volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));

    //! Interpolate the voxel values.
    const float voxelValue_00 = voxelValue_000 + fractionalCoordinates.x * (voxelValue_001 - voxelValue_000);
    const float voxelValue_01 = voxelValue_010 + fractionalCoordinates.x * (voxelValue_011 - voxelValue_010);
    const float voxelValue_10 = voxelValue_100 + fractionalCoordinates.x * (voxelValue_101 - voxelValue_100);
    const float voxelValue_11 = voxelValue_110 + fractionalCoordinates.x * (voxelValue_111 - voxelValue_110);
    const float voxelValue_0  = voxelValue_00  + fractionalCoordinates.y * (voxelValue_01  - voxelValue_00 );
    const float voxelValue_1  = voxelValue_10  + fractionalCoordinates.y * (voxelValue_11  - voxelValue_10 );
    const float volumeSample  = voxelValue_0   + fractionalCoordinates.z * (voxelValue_1   - voxelValue_0  );
    return(volumeSample);

}

inline varying uint32 BrickedVolume_getVoxelAddress(BrickedVolume *uniform volume, const varying vec3i &index) {

    //! Compute the 3D index of the volume brick containing the voxel.
    const vec3i brickIndex = index >> BRICK_VOXEL_WIDTH_BITCOUNT;

    //! Compute the 1D address of the volume brick containing the voxel.
    const uint32 brickAddress = brickIndex.x + volume->brickCount.x * (brickIndex.y + volume->brickCount.y * (uint32) brickIndex.z);

    //! Compute the 3D offset of the voxel in the volume brick.
    const vec3i voxelOffset = bitwise_AND(index, BRICK_VOXEL_WIDTH - 1);

    //! Compute the 1D address of the voxel.
    return(brickAddress << 3 * BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.z << 2 * BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.y << BRICK_VOXEL_WIDTH_BITCOUNT | voxelOffset.x);

}

inline varying float BrickedVolumeFloat_getVoxel(void *uniform pointer, const varying vec3i &index) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Cast to the actual voxel type.
    float *uniform voxelData = (float *uniform) volume->voxelData;

    //! Compute the 1D address of the voxel in the volume.
    const uint32 voxelAddress = BrickedVolume_getVoxelAddress(volume, index);

    //! The voxel value at the 1D address.
    return(voxelData[voxelAddress]);

}

inline varying float BrickedVolumeUChar_getVoxel(void *uniform pointer, const varying vec3i &index) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Cast to the actual voxel type.
    uint8 *uniform voxelData = (uint8 *uniform) volume->voxelData;

    //! Compute the 1D address of the voxel in the volume.
    const uint32 voxelAddress = BrickedVolume_getVoxelAddress(volume, index);

    //! The voxel value at the 1D address.
    return(voxelData[voxelAddress] / 255.0f);

}

inline void BrickedVolumeFloat_setVoxel(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Cast to the actual voxel type.
    float *uniform voxelData = (float *uniform) volume->voxelData;  const float *uniform sourceData = (const float *uniform) source;

    //! Compute the 1D address of the voxel in the volume.
    const uint32 voxelAddress = BrickedVolume_getVoxelAddress(volume, index + offset);

    //! Store the voxel value at the 1D address.
    voxelData[voxelAddress] = sourceData[(offset.z * volume->volumeDimensions.y + offset.y) * volume->volumeDimensions.x + offset.x];

}

inline void BrickedVolumeUChar_setVoxel(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const varying vec3i &offset) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Cast to the actual voxel type.
    uint8 *uniform voxelData = (uint8 *uniform) volume->voxelData;  const uint8 *uniform sourceData = (const uint8 *uniform) source;

    //! Compute the 1D address of the voxel in the volume.
    const uint32 voxelAddress = BrickedVolume_getVoxelAddress(volume, index + offset);

    //! Store the voxel value at the 1D address.
    voxelData[voxelAddress] = sourceData[(offset.z * volume->volumeDimensions.y + offset.y) * volume->volumeDimensions.x + offset.x];

}

export void BrickedVolume_allocateMemory(void *uniform pointer) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Memory may already have been allocated.
    if (volume->voxelData != NULL) return;

    //! Volume size in voxels with padding.
    const uniform uint32 voxelCount = volume->brickCount.x * volume->brickCount.y * volume->brickCount.z * BRICK_VOXEL_COUNT;

    //! Allocate storage for the voxel data.
    volume->voxelData = (volume->voxelType == OSP_FLOAT) ? (void *uniform)(uniform new uniform float[voxelCount]) : (void *uniform)(uniform new uniform uint8[voxelCount]);

}

export void *uniform BrickedVolume_createInstance(const uniform int voxelType) {

    //! The volume container.
    BrickedVolume *uniform volume = uniform new uniform BrickedVolume;

    //! Store the voxel type.
    volume->voxelType = (OSPDataType) voxelType;

    //! Store the voxel size.
    volume->voxelSize = (volume->voxelType == OSP_FLOAT) ? sizeof(uniform float) : sizeof(uniform uint8);

    //! Function to compute the gradient at a sample location in the volume.
    volume->inherited.computeGradient = BrickedVolume_computeGradient;

    //! Function to sample the volume.
    volume->inherited.computeSample = BrickedVolume_computeSample;

    //! Function to get a voxel value.
    volume->getVoxel = (volume->voxelType == OSP_FLOAT) ? BrickedVolumeFloat_getVoxel : BrickedVolumeUChar_getVoxel;

    //! Function to set a voxel value.
    volume->setVoxel = (volume->voxelType == OSP_FLOAT) ? BrickedVolumeFloat_setVoxel : BrickedVolumeUChar_setVoxel;

    //! Indicate that memory allocation is needed.
    volume->voxelData = NULL;  return(volume);

}

export void BrickedVolume_getVolumeSizeWithPadding(void *uniform pointer, uniform uint64 &value) {

    //! Cast to the actual Volume type.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! The volume size in bytes padded out to the next full brick.
    value = (uint64) volume->brickCount.x * volume->brickCount.y * volume->brickCount.z * volume->voxelSize * BRICK_VOXEL_COUNT;

}

export void BrickedVolume_setGammaCorrection(void *uniform pointer, const uniform vec2f &value) {

    //! Cast to the actual Volume type.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Set the gamma correction coefficient and exponent.
    volume->inherited.gammaCorrection = value;

}

export void BrickedVolume_setRegion(void *uniform pointer, const void *uniform source, const uniform vec3i &index, const uniform vec3i &count) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Copy voxel data from memory into the volume.
    foreach (z = 0 ... count.z, y = 0 ... count.y, x = 0 ... count.x) { const vec3i offset = make_vec3i(x, y, z);  volume->setVoxel(volume, source, index, offset); }

}

export void BrickedVolume_setStepSize(void *uniform pointer, const uniform float value) {

    //! Cast to the actual Volume type.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Set the step size for ray casting based renderers.
//  volume->inherited.stepSize = value;

}

export void BrickedVolume_setTransferFunction(void *uniform pointer, void *uniform value) {

    //! Cast to the actual Volume type.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Set the color and opacity transfer function.
    volume->inherited.transferFunction = (TransferFunction *uniform) value;

}

export void BrickedVolume_setVolumeDimensions(void *uniform pointer, const uniform vec3i &value) {

    //! Cast to the actual Volume subtype.
    BrickedVolume *uniform volume = (BrickedVolume *uniform) pointer;

    //! Set the step size for ray casting based renderers.
    volume->volumeDimensions = value;

    //! The largest floating point value still inside the volume.
    volume->inherited.upperBound = nextafter(volume->volumeDimensions - 1, make_vec3i(0));

    //! Volume size in bricks per dimension with padding to the nearest brick.
    volume->brickCount = (volume->volumeDimensions + BRICK_VOXEL_WIDTH - 1) / BRICK_VOXEL_WIDTH;

}

