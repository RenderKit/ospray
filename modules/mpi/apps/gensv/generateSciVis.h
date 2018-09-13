// ======================================================================== //
// Copyright 2017-2018 Intel Corporation                                    //
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

#pragma once

#include "ospcommon/FileName.h"
#include "ospcommon/containers/AlignedVector.h"
// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Geometry.h"
#include "ospray/ospray_cpp/Volume.h"

namespace gensv {
  using namespace ospcommon;

  // Compute an X x Y x Z grid to have num bricks,
  // only gives a nice grid for numbers with even factors since
  // we don't search for factors of the number, we just try dividing by two
  vec3i computeGrid(int num);

  /* This function generates the rank's local geometry within its
   * volume's bounding box. The bbox represents say its simulation
   * or owned data region.
   */
  ospray::cpp::Geometry makeSpheres(const box3f &bbox, const size_t numSpheres,
                                    const float sphereRadius,
                                    const bool transparent = false);

  struct LoadedVolume {
    ospray::cpp::Volume volume;
    ospray::cpp::TransferFunction tfcn;
    box3f bounds;
    vec3f ghostGridOrigin;

    LoadedVolume();
  };

  /* Generate this rank's volume data. The volumes are placed in
   * cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * The coloring is based on color-mapping the ranks id. Each region
   * can have more than one brick in the total gridding.
   * The region occupied by the volume is then used to be the rank's
   * overall region bounds and will be the bounding box for the
   * generated geometry as well.
   * Returns the ghostGridOrigin of the volume which may be outside the bounding
   * box, due to the ghost voxels.
   */
  LoadedVolume makeVolume();

  /* Generate bricks of volume data based on some gridding. The volumes are
   * placed in cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * The coloring is based on color-mapping the ranks id. Each region
   * can have more than one brick in the total gridding.
   * The region occupied by the volume is then used to be the rank's
   * overall region bounds and will be the bounding box for the
   * generated geometry as well.
   * Returns the ghostGridOrigin of the volume which may be outside the bounding
   * box, due to the ghost voxels.
   */
  containers::AlignedVector<LoadedVolume> makeVolumes(const size_t firstBrick,
                                                      const size_t numMine,
                                                      const size_t numBricks);

  /* Load this rank's volume data. The volumes are placed in
   * cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * Returns the ghostGridOrigin of the volume which may be outside the bounding
   * box, due to the ghost voxels.
   */
  LoadedVolume loadVolume(const FileName &file, const vec3i &dimensions,
                          const std::string &dtype, const vec2f &valueRange);
}

