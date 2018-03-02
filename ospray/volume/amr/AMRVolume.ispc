// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// ours
//#include "CellRef.ih"
// #include "AMRCommon.h"
#include "AMRVolume.ih"
#include "AMR.ih"
// ospray
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// embree
#include "embree2/rtcore.isph"

// ------------------------------------------------------------------
//! The current level at the given sample location in world coordinates.
static varying float AMR_level(void *uniform _self, const varying vec3f &pos)
{
  AMRVolume *uniform self = (AMRVolume *uniform) _self;
  return self->computeSampleLevel(self, pos);
}

// ------------------------------------------------------------------
//! The gradient at the given sample location in world coordinates.
static varying vec3f AMR_gradient(void *uniform _self, const varying vec3f &pos)
{
  // Cast to the actual Volume subtype.
  AMRVolume *uniform volume = (AMRVolume *uniform) _self;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep = make_vec3f(volume->super.samplingStep*.1f);  //Carson TODO: determine correct offset

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  // Forward differences.

  // Sample at gradient location.
  float sample = volume->super.sample(volume, pos);

  // Gradient magnitude in the X direction.
  gradient.x = volume->super.sample(volume, pos + make_vec3f(gradientStep.x, 0.0f, 0.0f)) - sample;

  // Gradient magnitude in the Y direction.
  gradient.y = volume->super.sample(volume, pos + make_vec3f(0.0f, gradientStep.y, 0.0f)) - sample;

  // Gradient magnitude in the Z direction.
  gradient.z = volume->super.sample(volume, pos + make_vec3f(0.0f, 0.0f, gradientStep.z)) - sample;

  // This approximation may yield image artifacts.
  return(gradient / gradientStep);
}

// adaptively step the ray based on the log of the
// current AMR level cell size.
// ray.time is set to interval length of intersected sample
inline void AMR_stepRay(void *uniform _volume,
                        varying Ray &ray,
                        const varying float samplingRate)
{
  // Cast to the actual Volume subtype.
  AMRVolume *uniform volume = (AMRVolume *uniform) _volume;

  //sample AMR structure to find size of current and next leaf sizes,
  // take the smallest reasonable step based on leaf size
  //TODO: Carson:  it would be significantly faster to return this in the
  // a single sampling step and adjust as necessary
  const float level1 = AMR_level(volume, ray.org + ray.dir*ray.t0);
  const float level2 = AMR_level(volume, ray.org + ray.dir*(ray.t0+ray.time));
  const float level = min(level1,level2);
  float rate = 1.f;
  if (level < 1.0f)
  {
    // take log of value.  otherwise step sizes get microscopically small.
    float nlevel = log(level)*-1.0f;
    nlevel = max(1.f/nlevel, 0.07f);
    rate = nlevel/samplingRate;
  }
  else
    rate = 1.0f/samplingRate;

  // The recommended step size for ray casting based volume renderers.
  const varying float step = volume->super.samplingStep * rate;

  // Compute the next hit point using a spatial acceleration structure.
  ray.t0 += step;
  ray.time = step;
}

export void *uniform AMRVolume_create(void *uniform cppE)
{
  AMRVolume *uniform self = uniform new uniform AMRVolume;
  return self;
}

export void AMRVolume_computeValueRangeOfLeaf(void *uniform _self,
                                              uniform int leafID)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;

  AMR *uniform amr            = &self->amr;
  AMRLeaf *uniform leaf       = amr->leaf + leafID;
  AMRBrick *uniform brick     = leaf->brickList[0];
  uniform float leafCellWidth = brick->cellWidth;
  uniform vec3f leafSize      = brick->bounds.upper - brick->bounds.lower;
  uniform vec3i leafCells     = make_vec3i((leafSize + 0.5f*leafCellWidth)
                                           * rcp(leafCellWidth));

  uniform vec3i numSamplePoints = 2*leafCells+1;
  for (uniform int iz=0;iz<numSamplePoints.z;iz++)
    for (uniform int iy=0;iy<numSamplePoints.y;iy++)
      for (varying int ix=0;ix<numSamplePoints.x;ix+=programCount) {
        vec3f relPos = make_vec3f(ix,iy,iz) / make_vec3f(numSamplePoints);
        vec3f samplePos = lerp(leaf->bounds,relPos);
        float sampleValue = self->super.sample(_self,samplePos);
        extend(leaf->valueRange,sampleValue);
      }
}

inline void AMRVolume_transformLocalToWorld(const AMRVolume *uniform volume, const varying vec3f &localCoordinates, varying vec3f &worldCoordinates)
{
  worldCoordinates = volume->gridOrigin + localCoordinates * volume->gridSpacing;
}

inline void AMRVolume_transformWorldToLocal(const AMRVolume *uniform volume, const varying vec3f &worldCoordinates, varying vec3f &localCoordinates)
{
  localCoordinates = rcp(volume->gridSpacing) * (worldCoordinates - volume->gridOrigin);
}

export void AMRVolume_setAMR(void *uniform _self,
                             uniform int numNodes, void *uniform _node,
                             uniform int numLeaves, void *uniform _leaf,
                             uniform int numLevels, void *uniform _level,
                             const uniform int voxelType,
                             const uniform box3f &worldBounds
                             )
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;

  self->amr.worldBounds          = worldBounds;
  self->amr.maxValidPos          = nextafter(worldBounds.upper,-1.f);
  self->amr.node                 = (KDTreeNode *uniform)_node;
  self->amr.numNodes             = numNodes;
  self->amr.leaf                 = (AMRLeaf *uniform)_leaf;
  self->amr.numLeaves            = numLeaves;
  self->amr.level                = (AMRLevel *uniform)_level;
  self->amr.finestLevel          = self->amr.level+numLevels-1;
  self->amr.numLevels            = numLevels;
  self->amr.finestLevelCellWidth = self->amr.level[numLevels-1].cellWidth;

  if (voxelType == OSP_UCHAR) {
    self->amr.getVoxel = AMR_getVoxel_uint8_32;
  } else if (voxelType == OSP_SHORT) {
    self->amr.getVoxel = AMR_getVoxel_int16_32;
  } else if (voxelType == OSP_USHORT) {
    self->amr.getVoxel = AMR_getVoxel_uint16_32;
  } else if (voxelType == OSP_FLOAT) {
    self->amr.getVoxel = AMR_getVoxel_float_32;
  } else if (voxelType == OSP_DOUBLE) {
    self->amr.getVoxel = AMR_getVoxel_double_32;
  } else {
      print("#osp:amrVolume unsupported voxelType");
      return;
  }
}

export void AMRVolume_set(void *uniform _self,
                          uniform box3f &worldBounds,
                          uniform float samplingStep,
                          const uniform vec3f &gridOrigin,
                          const uniform vec3f &gridSpacing
                          )
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;

  self->super.boundingBox       =
    make_box3f(gridOrigin+worldBounds.lower, worldBounds.lower + gridOrigin +
    (worldBounds.upper-worldBounds.lower)*gridSpacing);
  self->super.volumeClippingBox = self->super.boundingBox;
  self->super.samplingStep      = samplingStep;
  self->super.stepRay           = &AMR_stepRay;
  self->super.computeGradient   = &AMR_gradient;
  self->transformLocalToWorld = AMRVolume_transformLocalToWorld;
  self->transformWorldToLocal = AMRVolume_transformWorldToLocal;

  self->gridSpacing = gridSpacing;
  self->gridOrigin = gridOrigin;
}
