// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "volume/VisItStructuredVolume.ih"
#include "volume/VisItGridAccelerator.ih"

// //! Bit count used to represent the brick width.
// #define BRICK_WIDTH_BITCOUNT (4)
// //! Brick width in grid cells.
// #define BRICK_WIDTH (1 << BRICK_WIDTH_BITCOUNT)
// //! Brick size in cells.
// #define BRICK_CELL_COUNT (BRICK_WIDTH * BRICK_WIDTH * BRICK_WIDTH)
// //! Bit count used to represent the grid cell width.
// #define CELL_WIDTH_BITCOUNT (4)
// //! Grid cell width in volumetric elements.
// #define CELL_WIDTH (1 << CELL_WIDTH_BITCOUNT)

inline varying float VisItStructuredVolume_computeSample(void *uniform _volume, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  VisItStructuredVolume *uniform volume = (VisItStructuredVolume *uniform) _volume;

  // Transform the sample location into the local coordinate system.
  vec3f localCoordinates;
  volume->transformWorldToLocal(volume, worldCoordinates, localCoordinates);

  // Coordinates outside the volume are clamped to the volume bounds.
  const vec3f clampedLocalCoordinates = clamp(localCoordinates, make_vec3f(0.0f),
                                              volume->localCoordinatesUpperBound);

  // Lower and upper corners of the box straddling the voxels to be interpolated.
  const vec3i voxelIndex_0 = integer_cast(clampedLocalCoordinates);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  // Fractional coordinates within the lower corner voxel used during interpolation.
  const vec3f fractionalLocalCoordinates = clampedLocalCoordinates - float_cast(voxelIndex_0);

  // Look up the voxel values to be interpolated.
  float voxelValue_000;
  float voxelValue_001;
  float voxelValue_010;
  float voxelValue_011;
  float voxelValue_100;
  float voxelValue_101;
  float voxelValue_110;
  float voxelValue_111;
  volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_000);
  volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_001);
  volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_010);
  volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_011);
  volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_100);
  volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_101);
  volume->getVoxel(volume, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_110);
  volume->getVoxel(volume, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_111);

  // Interpolate the voxel values.
  const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

  return volumeSample;
}

inline varying vec3f VisItStructuredVolume_computeGradient(void *uniform _volume, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  VisItStructuredVolume *uniform volume = (VisItStructuredVolume *uniform) _volume;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep = volume->gridSpacing;

  // The gradient will be computed using central differences.
  varying vec3f gradient;

#if 0
  // Central differences.

  // Gradient magnitude in the X direction.
  gradient.x = volume->super.computeSample(volume, location + make_vec3f(gradientStep.x, 0.0f, 0.0f)) - volume->super.computeSample(volume, location + make_vec3f(-gradientStep.x, 0.0f, 0.0f));

  // Gradient magnitude in the Y direction.
  gradient.y = volume->super.computeSample(volume, location + make_vec3f(0.0f, gradientStep.y, 0.0f)) - volume->super.computeSample(volume, location + make_vec3f(0.0f, -gradientStep.y, 0.0f));

  // Gradient magnitude in the Z direction.
  gradient.z = volume->super.computeSample(volume, location + make_vec3f(0.0f, 0.0f, gradientStep.z)) - volume->super.computeSample(volume, location + make_vec3f(0.0f, 0.0f, -gradientStep.z));

  // This approximation may yield image artifacts.
  return(0.5f * gradient / gradientStep);
#else
  // Forward differences.

  // Sample at gradient location.
  float sample = volume->super.computeSample(volume, worldCoordinates);

  // Gradient magnitude in the X direction.
  gradient.x = volume->super.computeSample(volume, worldCoordinates + make_vec3f(gradientStep.x, 0.0f, 0.0f)) - sample;

  // Gradient magnitude in the Y direction.
  gradient.y = volume->super.computeSample(volume, worldCoordinates + make_vec3f(0.0f, gradientStep.y, 0.0f)) - sample;

  // Gradient magnitude in the Z direction.
  gradient.z = volume->super.computeSample(volume, worldCoordinates + make_vec3f(0.0f, 0.0f, gradientStep.z)) - sample;

  // This approximation may yield image artifacts.
  return(gradient / gradientStep);
#endif
}

// ray.time is set to interval length of intersected sample
inline void VisItStructuredVolume_intersect(void *uniform _volume, varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  VisItStructuredVolume *uniform volume = (VisItStructuredVolume *uniform) _volume;

  // The recommended step size for ray casting based volume renderers.
  const uniform float step = volume->super.samplingStep / volume->super.samplingRate;

  // // Compute the next hit point using a spatial acceleration structure.
  // VisItGridAccelerator_intersect(volume->accelerator, step, ray);

  // The associated volume.
  // -- ray
  // -- step (class field)

  // Tentatively advance the ray.
  ray.t0 += step;
  ray.time = step;

  // const vec3f ray_rdir = rcp(ray.dir);
  // // sign of direction determines near/far index
  // const vec3i nextCellIndex = make_vec3i(1 - (intbits(ray.dir.x) >> 31),
  //                                        1 - (intbits(ray.dir.y) >> 31),
  //                                        1 - (intbits(ray.dir.z) >> 31));

  // while (ray.t0 < ray.t) {
  //
  //   // // Compute the hit point in the local coordinate system.
  //   // vec3f localCoordinates;
  //   // volume->transformWorldToLocal(volume,
  //   //                               ray.org + ray.t0 * ray.dir,
  //   //                               localCoordinates);
  //   //
  //   // // Compute the 3D index of the cell containing the hit point.
  //   // vec3i cellIndex = integer_cast(localCoordinates) >> CELL_WIDTH_BITCOUNT;
  //   //
  //   // // If we visited this cell before then it must not be empty.
  //   // if (ray.geomID == cellIndex.x &&
  //   //     ray.primID == cellIndex.y &&
  //   //     ray.instID == cellIndex.z)
  //   //   return;
  //   //
  //   // // Track the hit cell.
  //   // ray.geomID = cellIndex.x;
  //   // ray.primID = cellIndex.y;
  //   // ray.instID = cellIndex.z;
  //   //
  //   // // Get the volumetric value range of the cell.
  //   // vec2f cellRange;
  //   // VisItGridAccelerator_getCellRange(accelerator, cellIndex, cellRange);
  //   //
  //   // // Get the maximum opacity in the volumetric value range.
  //   // float maximumOpacity =
  //   //     volume->super.transferFunction->getMaxOpacityInRange(volume->super.transferFunction,
  //   //                                                          cellRange);
  //   //
  //   // // Return the hit point if the grid cell is not fully transparent.
  //   // if (maximumOpacity > 0.0f) 
  //   // {
  //   //   return;
  //   // }
  //   //
  //   // // Exit bound of the grid cell in world coordinates.
  //   // vec3f farBound;
  //   // volume->transformLocalToWorld(volume,
  //   //                               float_cast(cellIndex +
  //   //                                          nextCellIndex <<
  //   //                                          CELL_WIDTH_BITCOUNT),
  //   //                               farBound);
  //   //
  //   // // Identify the distance along the ray to the exit points on the cell.
  //   // const vec3f maximum = ray_rdir * (farBound - ray.org);
  //   // const float exitDist = min(min(ray.t, maximum.x),
  //   //                            min(maximum.y, maximum.z));
  //   //
  //   // // Advance the ray so the next hit point will be outside the empty cell.
  //   // ray.t0 += ceil(abs(exitDist - ray.t0) / step) * step;
  // }
}

// ray.time is set to interval length of intersected sample
inline void VisItStructuredVolume_intersectAdaptive(void *uniform _volume, varying Ray &ray, const varying float samplingRate)
{
  // Cast to the actual Volume subtype.
  VisItStructuredVolume *uniform volume = (VisItStructuredVolume *uniform) _volume;

  // The recommended step size for ray casting based volume renderers.
  const varying float step = volume->super.samplingStep / samplingRate;

  // // Compute the next hit point using a spatial acceleration structure.
  // VisItGridAccelerator_intersectAdaptive(volume->accelerator, step, ray);

  // Tentatively advance the ray.
  ray.t0 += step;
  ray.time = step;

  // const vec3f ray_rdir = rcp(ray.dir);
  // // sign of direction determines near/far index
  // const vec3i nextCellIndex = make_vec3i(1 - (intbits(ray.dir.x) >> 31),
  //                                        1 - (intbits(ray.dir.y) >> 31),
  //                                        1 - (intbits(ray.dir.z) >> 31));

  // while (ray.t0 < ray.t) {
  //   // Compute the hit point in the local coordinate system.
  //   vec3f localCoordinates;
  //   volume->transformWorldToLocal(volume,
  //                                 ray.org + ray.t0 * ray.dir,
  //                                 localCoordinates);

  //   // Compute the 3D index of the cell containing the hit point.
  //   vec3i cellIndex = integer_cast(localCoordinates) >> CELL_WIDTH_BITCOUNT;

  //   // If we visited this cell before then it must not be empty.
  //   if (ray.geomID == cellIndex.x &&
  //       ray.primID == cellIndex.y &&
  //       ray.instID == cellIndex.z)
  //     return;

  //   // Track the hit cell.
  //   ray.geomID = cellIndex.x;
  //   ray.primID = cellIndex.y;
  //   ray.instID = cellIndex.z;

  //   // return always
  //   return;

  //   // // Get the volumetric value range of the cell.
  //   // vec2f cellRange;
  //   // VisItGridAccelerator_getCellRange(accelerator, cellIndex, cellRange);

  //   // // Get the maximum opacity in the volumetric value range.
  //   // float maximumOpacity =
  //   //     volume->super.transferFunction->getMaxOpacityInRange(volume->super.transferFunction,
  //   //                                                          cellRange);
  //   //
  //   // // Return the hit point if the grid cell is not fully transparent.
  //   // if (maximumOpacity > 0.0f) 
  //   // {
  //   //   //adaptive sampling based on maximum opacity in cell
  //   //   //ray.t0 -= step;
  //   //   //float adaptiveStep = min(1.f/(maximumOpacity), 2.f)*step;
  //   //   //print("maximum %\n", maximumOpacity);
  //   //   //adaptiveStep = step;
  //   //   //ray.t0 += adaptiveStep;
  //   //   //ray.time = adaptiveStep;
  //   //   return;
  //   // }
  //   //
  //   // // Exit bound of the grid cell in world coordinates.
  //   // vec3f farBound;
  //   // volume->transformLocalToWorld(volume,
  //   //                               float_cast(cellIndex +
  //   //                                          nextCellIndex <<
  //   //                                          CELL_WIDTH_BITCOUNT),
  //   //                               farBound);
  //   //
  //   // // Identify the distance along the ray to the exit points on the cell.
  //   // const vec3f maximum = ray_rdir * (farBound - ray.org);
  //   // const float exitDist = min(min(ray.t, maximum.x),
  //   //                            min(maximum.y, maximum.z));
  //   //
  //   // // Advance the ray so the next hit point will be outside the empty cell.
  //   // const float dist = ceil(abs(exitDist - ray.t0) / step) * step;
  //   // ray.t0 += dist;
  //   // ray.time = dist;
  // }
}

inline void VisItStructuredVolume_intersectIsosurface(void *uniform _volume, uniform float *uniform isovalues, uniform int numIsovalues, varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  VisItStructuredVolume *uniform volume = (VisItStructuredVolume *uniform) _volume;

  // The nominal step size for ray casting based volume renderers, not considering the sampling rate.
  const uniform float step = volume->super.samplingStep;

  // // Compute the next hit point using a spatial acceleration structure.
  // VisItGridAccelerator_intersectIsosurface(volume->accelerator, step, isovalues, numIsovalues, ray);
  
  // Tentatively advance the ray.
  ray.t0 += step;
  if (ray.t0 >= ray.t) return;

  // // Compute the hit point in the local coordinate system.
  // vec3f localCoordinates;
  // volume->transformWorldToLocal(volume,
  //                               ray.org + ray.t0 * ray.dir,
  //                               localCoordinates);

  // // Compute the 3D index of the cell containing the hit point.
  // vec3i cellIndex = integer_cast(localCoordinates) >> CELL_WIDTH_BITCOUNT;

  // // If we visited this cell before then it must not be empty.
  // if (ray.geomID == cellIndex.x &&
  //     ray.primID == cellIndex.y &&
  //     ray.instID == cellIndex.z)
  //   return;

  // // Track the hit cell.
  // ray.geomID = cellIndex.x;
  // ray.primID = cellIndex.y;
  // ray.instID = cellIndex.z;

  // // // Get the volumetric value range of the cell.
  // // vec2f cellRange;
  // // VisItGridAccelerator_getCellRange(accelerator, cellIndex, cellRange);
  // //
  // // // Return the hit point if the grid cell contains an isovalue.
  // // for (uniform int i=0; i<numIsovalues; i++) {
  // //   if (isovalues[i] >= cellRange.x && isovalues[i] <= cellRange.y)
  // //     return;
  // // }
  // //
  // // // Bounds of the grid cell in world coordinates.
  // // box3f cellBounds = VisItGridAccelerator_getCellBounds(accelerator, cellIndex);
  // //
  // // // Identify the distance along the ray to the entry and exit points on the
  // // // cell.
  // // vec2f cellInterval = VisItGridAccelerator_intersectCell(cellBounds, ray);
  // //
  // // // Advance the ray so the next hit point will be outside the empty cell.
  // // ray.t0 += floor(abs(cellInterval.y - ray.t0) / step) * step - step;
  // //
  // // // Rinse and repeat.
  // // VisItGridAccelerator_intersectIsosurface(accelerator,
  // //                                     step,
  // //                                     isovalues,
  // //                                     numIsovalues,
  // //                                     ray);
}

inline void VisItStructuredVolume_transformLocalToWorld(VisItStructuredVolume *uniform volume, const varying vec3f &localCoordinates, varying vec3f &worldCoordinates)
{
  worldCoordinates = volume->gridOrigin + localCoordinates * volume->gridSpacing;
}

inline void VisItStructuredVolume_transformWorldToLocal(VisItStructuredVolume *uniform volume, const varying vec3f &worldCoordinates, varying vec3f &localCoordinates)
{
  localCoordinates = rcp(volume->gridSpacing) * (worldCoordinates - volume->gridOrigin);
}

void VisItStructuredVolume_Constructor(VisItStructuredVolume *uniform volume,
                                  /*! pointer to the c++-equivalent class of this entity */
                                  void *uniform cppEquivalent,
                                  const uniform vec3i &dimensions
                                  )
{
  Volume_Constructor(&volume->super,cppEquivalent);

  volume->dimensions = dimensions;
  volume->accelerator = NULL;
  volume->localCoordinatesUpperBound = nextafter(volume->dimensions - 1, make_vec3i(0));
  volume->getVoxel = NULL;
  volume->transformLocalToWorld = VisItStructuredVolume_transformLocalToWorld;
  volume->transformWorldToLocal = VisItStructuredVolume_transformWorldToLocal;

  volume->super.boundingBox = make_box3f(volume->gridOrigin, volume->gridOrigin + make_vec3f(volume->dimensions - 1) * volume->gridSpacing);
  volume->super.computeSample = VisItStructuredVolume_computeSample;
  volume->super.computeGradient = VisItStructuredVolume_computeGradient;
  volume->super.intersect = VisItStructuredVolume_intersect;
  volume->super.intersectAdaptive = VisItStructuredVolume_intersectAdaptive;
  volume->super.intersectIsosurface = VisItStructuredVolume_intersectIsosurface;
}

export void VisItStructuredVolume_setGridOrigin(void *uniform _self, const uniform vec3f &value)
{
  uniform VisItStructuredVolume *uniform self = (uniform VisItStructuredVolume *uniform)_self;
  self->gridOrigin = value;

  // Update bounding box for new grid definition.
  self->super.boundingBox = make_box3f(self->gridOrigin, self->gridOrigin + make_vec3f(self->dimensions - 1) * self->gridSpacing);
}

export void VisItStructuredVolume_setGridSpacing(void *uniform _self, const uniform vec3f &value)
{
  uniform VisItStructuredVolume *uniform self = (uniform VisItStructuredVolume *uniform)_self;
  self->gridSpacing = value;

  // Update nominal sampling step size for new grid definition.
  self->super.samplingStep = reduce_min(self->gridSpacing);

  // Update bounding box for new grid definition.
  self->super.boundingBox = make_box3f(self->gridOrigin, self->gridOrigin + make_vec3f(self->dimensions - 1) * self->gridSpacing);
}

export void *uniform VisItStructuredVolume_createAccelerator(void *uniform _self)
{
  // Cast to the actual Volume type.
  VisItStructuredVolume *uniform self = (VisItStructuredVolume *uniform)_self;

  // Delete the current accelerator if it exists.
  if(self->accelerator) VisItGridAccelerator_destroy(self->accelerator);

  // Set the accelerator structure field.
  self->accelerator = VisItGridAccelerator_createInstance(&self->super);

  return self->accelerator;
}
