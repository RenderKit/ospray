#pragma once

#include "ospcommon/FileName.h"
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
                                    const float sphereRadius);

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
   * The coloring is based on color-mapping the ranks id.
   * The region occupied by the volume is then used to be the rank's
   * overall region bounds and will be the bounding box for the
   * generated geometry as well.
   * Returns the ghostGridOrigin of the volume which may be outside the bounding
   * box, due to the ghost voxels.
   */
  LoadedVolume makeVolume();

  /* Load this rank's volume data. The volumes are placed in
   * cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * Returns the ghostGridOrigin of the volume which may be outside the bounding
   * box, due to the ghost voxels.
   */
  LoadedVolume loadVolume(const FileName &file, const vec3i &dimensions,
                          const std::string &dtype, const vec2f &valueRange);
}

