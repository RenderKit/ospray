#pragma once

// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Geometry.h"
#include "ospray/ospray_cpp/Volume.h"

namespace gensv {
  using namespace ospcommon;

  //TODO: factor this into a reusable piece inside of ospcommon!!!!!!
  // helper function to write the rendered image as PPM file
  void writePPM(const std::string &fileName,
                const int sizeX, const int sizeY,
                const uint32_t *pixel);

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

  /* Generate this rank's volume data. The volumes are placed in
   * cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * The coloring is based on color-mapping the ranks id.
   * The region occupied by the volume is then used to be the rank's
   * overall region bounds and will be the bounding box for the
   * generated geometry as well.
   */
  std::pair<ospray::cpp::Volume, box3f> makeVolume();
}

