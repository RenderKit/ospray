//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/volume/BrickedVolumeUChar.ih"

#define BRICK_WIDTH_EXPONENT  (4)
#define BRICK_VOXEL_WIDTH     (1 << BRICK_WIDTH_EXPONENT)
#define BRICK_VOXEL_COUNT     (BRICK_VOXEL_WIDTH * BRICK_VOXEL_WIDTH * BRICK_VOXEL_WIDTH)

varying vec3f BrickedVolumeUChar_computeGradient(void *uniform pointer, const varying vec3f &coordinates) {

    //! Cast to the actual Volume subtype.
    BrickedVolumeUChar *uniform volume = (BrickedVolumeUChar *uniform) pointer;

    //! The gradient will be computed using central differences.
    varying vec3f gradient;

    //! Gradient magnitude in the X direction.
    gradient.x  = BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f( 1.0f, 0.0f, 0.0f));
    gradient.x -= BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f(-1.0f, 0.0f, 0.0f));

    //! Gradient magnitude in the Y direction.
    gradient.y  = BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f(0.0f,  1.0f, 0.0f));
    gradient.y -= BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f(0.0f, -1.0f, 0.0f));

    //! Gradient magnitude in the Z direction.
    gradient.z  = BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f,  1.0f));
    gradient.z -= BrickedVolumeUChar_computeSample(volume, coordinates + make_vec3f(0.0f, 0.0f, -1.0f));

    //! This approximation may yield image artifacts.
    return(gradient);

}

varying float BrickedVolumeUChar_computeSample(void *uniform pointer, const varying vec3f &coordinates) {

    //! Cast to the actual Volume subtype.
    BrickedVolumeUChar *uniform volume = (BrickedVolumeUChar *uniform) pointer;

    //! Coordinates outside the volume are clamped to the volume bounds.
    const vec3f clampedCoordinates = clamp(coordinates, make_vec3f(0.0f), volume->inherited.upperBound);

    //! Lower and upper corners of the box straddling the voxels to be interpolated.
    const vec3i voxelIndex_0 = integer_cast(clampedCoordinates);  const vec3i voxelIndex_1 = voxelIndex_0 + make_vec3i(1);

    //! Fractional coordinates within the lower corner voxel used during interpolation.
    const vec3f fractionalCoordinates = clampedCoordinates - float_cast(voxelIndex_0);

    //! Look up the voxel values to be interpolated.
    const float voxelValue_000 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
    const float voxelValue_001 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
    const float voxelValue_010 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
    const float voxelValue_011 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
    const float voxelValue_100 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
    const float voxelValue_101 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
    const float voxelValue_110 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
    const float voxelValue_111 = BrickedVolumeUChar_getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));

    //! Interpolate the voxel values.
    const float voxelValue_00 = voxelValue_000 + fractionalCoordinates.x * (voxelValue_001 - voxelValue_000);
    const float voxelValue_01 = voxelValue_010 + fractionalCoordinates.x * (voxelValue_011 - voxelValue_010);
    const float voxelValue_10 = voxelValue_100 + fractionalCoordinates.x * (voxelValue_101 - voxelValue_100);
    const float voxelValue_11 = voxelValue_110 + fractionalCoordinates.x * (voxelValue_111 - voxelValue_110);
    const float voxelValue_0  = voxelValue_00  + fractionalCoordinates.y * (voxelValue_01  - voxelValue_00 );
    const float voxelValue_1  = voxelValue_10  + fractionalCoordinates.y * (voxelValue_11  - voxelValue_10 );
    const float volumeSample  = voxelValue_0   + fractionalCoordinates.z * (voxelValue_1   - voxelValue_0  );
    return(volumeSample * (1.0f / 255.0f));

}

export void *uniform BrickedVolumeUChar_createInstance(const uniform vec3i &dimensions, const void *uniform function) {

    //! The volume container.
    BrickedVolumeUChar *uniform volume = uniform new BrickedVolumeUChar;

    //! Volume size in bricks per dimension with padding to the nearest brick.
    volume->brickCount = (dimensions + BRICK_VOXEL_WIDTH - 1) / BRICK_VOXEL_WIDTH;

    //! Volume size in voxels per dimension.
    volume->dimensions = dimensions;

    //! Function to compute the gradient at a sample location in the volume.
    volume->inherited.computeGradient = BrickedVolumeUChar_computeGradient;

    //! Function to sample the volume.
    volume->inherited.computeSample = BrickedVolumeUChar_computeSample;

    //! Color and opacity transfer function.
    volume->inherited.transferFunction = (TransferFunction *uniform) function;

    //! The largest floating point value still inside the volume.
    volume->inherited.upperBound = nextafter(volume->dimensions - 1, make_vec3i(0));

    //! Volume size in voxels with padding.
    const uniform int voxelCount = volume->brickCount.x * volume->brickCount.y * volume->brickCount.z * BRICK_VOXEL_COUNT;

    //! Allocate storage for the voxel data.
    volume->voxelData = uniform new uint8[voxelCount];  return(volume);

}

inline varying int BrickedVolumeUChar_getVoxelAddress(BrickedVolumeUChar *uniform volume, const varying vec3i &index) {

    //! Compute the 3D index of the volume brick containing the voxel.
    const vec3i brickIndex = index >> BRICK_WIDTH_EXPONENT;

    //! Compute the 1D address of the volume brick containing the voxel.
    const int brickAddress = brickIndex.x + volume->brickCount.x * (brickIndex.y + volume->brickCount.y * brickIndex.z);

    //! Compute the 3D offset of the voxel in the volume brick.
    const vec3i voxelOffset = bitwise_AND(index, BRICK_VOXEL_WIDTH - 1);

    //! Compute the 1D address of the voxel.
    return((((((brickAddress << BRICK_WIDTH_EXPONENT) | voxelOffset.z) << BRICK_WIDTH_EXPONENT) | voxelOffset.y) << BRICK_WIDTH_EXPONENT) | voxelOffset.x);

}

inline varying uint8 BrickedVolumeUChar_getVoxel(BrickedVolumeUChar *uniform volume, const varying vec3i &index) {

    //! Compute the 1D address of the voxel in the volume.
    const int voxelAddress = BrickedVolumeUChar_getVoxelAddress(volume, index);

    //! The voxel value at the 1D address.
    return(volume->voxelData[voxelAddress]);

}

export void BrickedVolumeUChar_setRegion(void *uniform pointer, const uint8 *uniform source, const uniform vec3i &index, const uniform vec3i &count) {

    //! Cast to the actual Volume subtype.
    BrickedVolumeUChar *uniform volume = (BrickedVolumeUChar *uniform) pointer;

    //! Copy voxel data from memory into the volume.
    foreach (z = 0 ... count.z, y = 0 ... count.y, x = 0 ... count.x) { const vec3i offset = make_vec3i(x, y, z);  BrickedVolumeUChar_setVoxel(volume, source, index, offset); }

}

inline void BrickedVolumeUChar_setVoxel(BrickedVolumeUChar *uniform volume, const uint8 *uniform source, const uniform vec3i &index, const varying vec3i &offset) {

    //! Compute the 1D address of the voxel in the volume.
    const int voxelAddress = BrickedVolumeUChar_getVoxelAddress(volume, index + offset);

    //! Store the voxel value at the 1D address.
    volume->voxelData[voxelAddress] = source[(offset.z * volume->dimensions.y + offset.y) * volume->dimensions.x + offset.x];

}

