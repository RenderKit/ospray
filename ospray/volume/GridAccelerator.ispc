//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/math/bbox.ih"
#include "ospray/volume/GridAccelerator.ih"

// namespace GridAccelerator {

    //! Bit count used to represent the brick width.
    #define BRICK_WIDTH_BITCOUNT (4)

    //! Brick width in grid cells.
    #define BRICK_WIDTH (1 << BRICK_WIDTH_BITCOUNT)

    //! Brick size in cells.
    #define BRICK_CELL_COUNT (BRICK_WIDTH * BRICK_WIDTH * BRICK_WIDTH)

    //! Bit count used to represent the grid cell width.
    #define CELL_WIDTH_BITCOUNT (4)

    //! Grid cell width in volumetric elements.
    #define CELL_WIDTH (1 << CELL_WIDTH_BITCOUNT)

    //! Get the encoded bit vector for a given cell.
    uint32 GridAccelerator_getCellCode(GridAccelerator *uniform accelerator, const varying vec3i &index);

    //! Bitwise OR an encoded bit vector with the vector stored for a given cell.
    void GridAccelerator_setCellCode(GridAccelerator *uniform accelerator, const varying vec3i &index, const uint32 code);

    //! Transform the given point in volume coordinates into world coordinates.
    inline vec3f GridAccelerator_transformLocalToWorld(GridAccelerator *uniform accelerator, const varying vec3f &coordinates)
        { return(coordinates / accelerator->volumeExtents); }

    //! Transform the given point into the local coordinate system of the volume.
    inline vec3f GridAccelerator_transformWorldToLocal(GridAccelerator *uniform accelerator, const varying vec3f &coordinates)
        { return(coordinates * accelerator->volumeExtents); }

// }

GridAccelerator *uniform GridAccelerator_createInstance() {

    //! The accelerator container.
    GridAccelerator *uniform accelerator = uniform new uniform GridAccelerator;

    //! Find the largest floating point number less than the value range bucket count.
    accelerator->rangeScale = nextafter(32.0f, 0.0f);  return(accelerator);

}

void GridAccelerator_encodeTransferFunction(GridAccelerator *uniform accelerator, TransferFunction *uniform transferFunction) {

    //! Encode the transfer function alpha values.
    accelerator->transferFunctionCode = 0;  for (uniform uint32 i=0 ; i < 32 ; i++) {

        //! The minimum value contained in the current value range bucket.
        const uniform float rangeMinimum = (i / accelerator->rangeScale) * (accelerator->valueRange.y - accelerator->valueRange.x) + accelerator->valueRange.x;

        //! The maximum value contained in the current value range bucket.
        const uniform float rangeMaximum = (nextafter(i + 1.0f, 0.0f) / accelerator->rangeScale) * (accelerator->valueRange.y - accelerator->valueRange.x) + accelerator->valueRange.x;

        //! Get the minimum and maximum alpha values for the given value range.
        const uniform vec2f alphaRange = transferFunction->getMinMaxAlphaInRange(transferFunction, rangeMinimum, rangeMaximum);

        //! Encode the maximum alpha value.
        if (alphaRange.y > 0.0f) accelerator->transferFunctionCode |= 1 << i;

    }

}

void GridAccelerator_encodeValue(GridAccelerator *uniform accelerator, const varying vec3f &coordinates, varying float value) {

    //! Compute the 3D index of the voxel containing the value, and the cell containing the voxel.
    const vec3i voxelIndex = integer_cast(coordinates);  const vec3i cellIndex = voxelIndex >> CELL_WIDTH_BITCOUNT;

    //! Values near cell bounds are added to adjacent cells to account for later interpolation.
    const vec3i cellCount = cellIndex - (max(voxelIndex - 1, 0) >> CELL_WIDTH_BITCOUNT) + 1;

    //! Project the value into the range [0.0, 1.0].
    float normalizedValue = clamp((value - accelerator->valueRange.x) / (accelerator->valueRange.y - accelerator->valueRange.x));

    //! Encode the value as a bit vector.
    uint32 valueCode = 1 << (uint32)(normalizedValue * accelerator->rangeScale);

    //! Add the value to the current and any adjacent cells.
//  for (size_t z=0 ; z < cellCount.z ; z++)
//      for (size_t y=0 ; y < cellCount.y ; y++)
//          for (size_t x=0 ; x < cellCount.x ; x++)
//              GridAccelerator_setCellCode(accelerator, cellIndex + make_vec3i(x, y, z), valueCode);

    GridAccelerator_setCellCode(accelerator, cellIndex, valueCode);

}

inline uint32 GridAccelerator_getCellAddress(GridAccelerator *uniform accelerator, const varying vec3i &index) {

    //! Compute the 3D index of the grid brick containing the cell.
    const vec3i brickIndex = index >> BRICK_WIDTH_BITCOUNT;

    //! Compute the 1D address of the grid brick containing the cell.
    const uint32 brickAddress = brickIndex.x + accelerator->brickCount.x * (brickIndex.y + accelerator->brickCount.y * (uint32) brickIndex.z);

    //! Compute the 3D offset of the cell in the grid brick.
    const vec3i cellOffset = bitwise_AND(index, BRICK_WIDTH - 1);

    //! Compute the 1D address of the cell.
    return(brickAddress << 3 * BRICK_WIDTH_BITCOUNT | cellOffset.z << 2 * BRICK_WIDTH_BITCOUNT | cellOffset.y << BRICK_WIDTH_BITCOUNT | cellOffset.x);

}

inline box3f GridAccelerator_getCellBounds(GridAccelerator *uniform accelerator, const varying vec3i &index) {

    //! Coordinates of the lower corner of the cell in world coordinates.
    const vec3f lower = GridAccelerator_transformLocalToWorld(accelerator, float_cast(index << CELL_WIDTH_BITCOUNT));

    //! Coordinates of the upper corner of the cell in world coordinates.
    const vec3f upper = GridAccelerator_transformLocalToWorld(accelerator, float_cast(index + 1 << CELL_WIDTH_BITCOUNT));

    //! The bounding box in world coordinates.
    return(make_box3f(lower, upper));

}

uint32 GridAccelerator_getCellCode(GridAccelerator *uniform accelerator, const varying vec3i &index) {

    //! Compute the 1D address of the cell in the grid.
    const uint32 cellAddress = GridAccelerator_getCellAddress(accelerator, index);

    //! The cell code at the 1D address.
    return(accelerator->cellCodes[cellAddress]);

}

inline vec2f GridAccelerator_intersectCell(const varying box3f &bounds, const varying Ray &ray) {

    //! Intersection interval minimum per axis.
    const vec3f minimum = rcp(ray.dir) * (bounds.lower - ray.org);

    //! Intersection interval maximum per axis.
    const vec3f maximum = rcp(ray.dir) * (bounds.upper - ray.org);

    //! Distance along the ray to the entry point.
    vec2f interval;  interval.x = max(max(ray.t0, min(minimum.x, maximum.x)), max(min(minimum.y, maximum.y), min(minimum.z, maximum.z)));

    //! Distance along the ray to the exit point.
    interval.y = min(min(ray.t1, max(minimum.x, maximum.x)), min(max(minimum.y, maximum.y), max(minimum.z, maximum.z)));  return(interval);

}

void GridAccelerator_intersect(GridAccelerator *uniform accelerator, uniform float step, varying Ray &ray) {

    //! Tentatively advance the ray.
    ray.t += step;  if (ray.t >= ray.t1) return;

    //! Compute the hit point in the local coordinate system.
    const vec3f coordinates = GridAccelerator_transformWorldToLocal(accelerator, ray.org + ray.t * ray.dir);

    //! Compute the 3D index of the cell containing the hit point.
    const vec3i cellIndex = integer_cast(coordinates) >> CELL_WIDTH_BITCOUNT;

    //! Return the hit point if the grid cell is not fully transparent.
    uint32 cellOpacity = GridAccelerator_getCellCode(accelerator, cellIndex) & accelerator->transferFunctionCode;  if (cellOpacity) return;

    //! Bounds of the grid cell in world coordinates.
    const box3f cellBounds = GridAccelerator_getCellBounds(accelerator, cellIndex);

    //! Identify the distance along the ray to the entry and exit points on the cell.
    const vec2f cellInterval = GridAccelerator_intersectCell(cellBounds, ray);

    //! Advance the ray so the next hit point will be outside the empty cell.
    ray.t += floor(abs(cellInterval.y - ray.t) / step) * step;

    //! Rinse and repeat.
    GridAccelerator_intersect(accelerator, step, ray);

}

void GridAccelerator_setCellCode(GridAccelerator *uniform accelerator, const varying vec3i &index, const uint32 code) {

    //! Compute the 1D address of the cell in the grid.
    const uint32 cellAddress = GridAccelerator_getCellAddress(accelerator, index);

    //! Store the cell code at the 1D address, assumes this code is not multithreaded.
    atomic_or_local(&accelerator->cellCodes[cellAddress], code);

}

void GridAccelerator_setVolumeDimensions(GridAccelerator *uniform accelerator, const uniform vec3i &volumeDimensions) {

    //! Grid size in cells per dimension after padding out the volume dimensions to the nearest cell.
    accelerator->gridDimensions = (volumeDimensions + CELL_WIDTH - 1) / CELL_WIDTH;

    //! Grid size in bricks per dimension after padding out the grid dimensions to the nearest brick.
    accelerator->brickCount = (accelerator->gridDimensions + BRICK_WIDTH - 1) / BRICK_WIDTH;

    //! Grid cell count with padding.
    const uniform size_t cellCount = accelerator->brickCount.x * accelerator->brickCount.y * accelerator->brickCount.z * BRICK_CELL_COUNT;

    //! Allocate storage for the encoded volumetric values per cell.
    accelerator->cellCodes = uniform new uniform uint32[cellCount];  memset(accelerator->cellCodes, 0, cellCount * sizeof(uniform uint32));

    //! The largest floating point value still inside the volume.
    accelerator->volumeExtents = nextafter(volumeDimensions - 1, make_vec3i(0));

}

