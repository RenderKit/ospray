#include <random>
#include <array>
#include "mpiCommon/MPICommon.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "ospray/ospray_cpp/Volume.h"
#include "raw_reader.h"
#include "generateSciVis.h"

namespace gensv {

  bool computeDivisor(int x, int &divisor)
  {
    int upperBound = std::sqrt(x);
    for (int i = 2; i <= upperBound; ++i) {
      if (x % i == 0) {
        divisor = i;
        return true;
      }
    }
    return false;
  }

  // Compute an X x Y x Z grid to have num bricks,
  // only gives a nice grid for numbers with even factors since
  // we don't search for factors of the number, we just try dividing by two
  vec3i computeGrid(int num)
  {
    vec3i grid(1);
    int axis = 0;
    int divisor = 0;
    while (computeDivisor(num, divisor)) {
      grid[axis] *= divisor;
      num /= divisor;
      axis = (axis + 1) % 3;
    }
    if (num != 1) {
      grid[axis] *= num;
    }
    return grid;
  }

  /* This function generates the rank's local geometry within its
   * volume's bounding box. The bbox represents say its simulation
   * or owned data region.
   */
  ospray::cpp::Geometry makeSpheres(const box3f &bbox, const size_t numSpheres,
                                    const float sphereRadius)
  {
    struct Sphere
    {
      vec3f org;
      int colorID {0};
    };

    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

    std::vector<Sphere> spheres(numSpheres);

    std::mt19937 rng;
    rng.seed(std::random_device()());

    // Generate spheres within this nodes volume, to keep the data disjoint.
    // We also leave some buffer space on the boundaries to avoid clipping
    // artifacts or needing duplication across nodes in the case a sphere
    // crosses a boundary. Note: Since we don't communicated ghost regions
    // among the nodes, we make sure not to generate any spheres which would
    // be clipped.
    std::uniform_real_distribution<float> dist_x(bbox.lower.x + sphereRadius,
                                                 bbox.upper.x - sphereRadius);
    std::uniform_real_distribution<float> dist_y(bbox.lower.y + sphereRadius,
                                                 bbox.upper.y - sphereRadius);
    std::uniform_real_distribution<float> dist_z(bbox.lower.z + sphereRadius,
                                                 bbox.upper.z - sphereRadius);

    for (auto &s : spheres) {
      s.org.x = dist_x(rng);
      s.org.y = dist_y(rng);
      s.org.z = dist_z(rng);
    }

    ospray::cpp::Data sphere_data(numSpheres * sizeof(Sphere), OSP_UCHAR,
                                  spheres.data());

    const float r = (numRanks - myRank) / numRanks;
    const float b = myRank / numRanks;
    const float g = myRank > numRanks / 2 ? 2 * r : 2 * b;
    vec4f color(r, g, b, 1.f);
    ospray::cpp::Data color_data(1, OSP_FLOAT4, &color);

    ospray::cpp::Geometry geom("spheres");
    geom.set("spheres", sphere_data);
    geom.set("color", color_data);
    geom.set("offset_colorID", int(sizeof(vec3f)));
    geom.set("radius", sphereRadius);
    geom.commit();

    return geom;
  }

  LoadedVolume::LoadedVolume() : volume(nullptr), tfcn("piecewise_linear") {
    const std::vector<vec3f> colors = {
      vec3f(0, 0, 0.56),
      vec3f(0, 0, 1),
      vec3f(0, 1, 1),
      vec3f(0.5, 1, 0.5),
      vec3f(1, 1, 0),
      vec3f(1, 0, 0),
      vec3f(0.5, 0, 0)
    };
    const std::vector<float> opacities = {0.0001, 1.0};
    ospray::cpp::Data colorsData(colors.size(), OSP_FLOAT3, colors.data());
    ospray::cpp::Data opacityData(opacities.size(), OSP_FLOAT, opacities.data());
    colorsData.commit();
    opacityData.commit();

    tfcn.set("colors", colorsData);
    tfcn.set("opacities", opacityData);
  }

  enum GhostFace {
    NEITHER_FACE = 0,
    POS_FACE = 1,
    NEG_FACE = 1 << 1,
  };

  /* Compute which faces of this brick we need to specify ghost voxels for,
   * to have correct interpolation at brick boundaries. Returns mask of
   * GhostFaces for x, y, z.
   */
  std::array<int, 3> computeGhostFaces(const vec3i &brickId, const vec3i &grid) {
    std::array<int, 3> faces = {{NEITHER_FACE, NEITHER_FACE, NEITHER_FACE}};
    for (size_t i = 0; i < 3; ++i) {
      if (brickId[i] < grid[i] - 1) {
        faces[i] |= POS_FACE;
      }
      if (brickId[i] > 0) {
        faces[i] |= NEG_FACE;
      }
    }
    return faces;
  }

  /* Generate this rank's volume data. The volumes are placed in
   * cells of the grid computed in 'computeGrid' based on the number
   * of ranks with each rank owning a specific cell in the gridding.
   * The coloring is based on color-mapping the ranks id.
   * The region occupied by the volume is then used to be the rank's
   * overall region bounds and will be the bounding box for the
   * generated geometry as well.
   */
  LoadedVolume makeVolume() {
    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

    LoadedVolume vol;
    vol.tfcn.set("valueRange", vec2f(0, numRanks - 1));
    vol.tfcn.commit();

    const vec3i volumeDims(128);
    const vec3i grid = computeGrid(numRanks);
    const vec3f gridSpacing = vec3f(1.f) / (vec3f(grid) * vec3f(volumeDims));
    const vec3i brickId(myRank % grid.x, (myRank / grid.x) % grid.y, myRank / (grid.x * grid.y));
    const vec3f gridOrigin = vec3f(brickId) * gridSpacing * vec3f(volumeDims);
    const std::array<int, 3> ghosts = computeGhostFaces(brickId, grid);
    vec3i ghostDims(0);
    for (size_t i = 0; i < 3; ++i) {
      if (ghosts[i] & POS_FACE) {
        ghostDims[i] += 1;
      }
      if (ghosts[i] & NEG_FACE) {
        ghostDims[i] += 1;
      }
    }
    const vec3i fullDims = volumeDims + ghostDims;
    const vec3i ghostOffset(ghosts[0] & NEG_FACE ? 1 : 0,
                               ghosts[1] & NEG_FACE ? 1 : 0,
                               ghosts[2] & NEG_FACE ? 1 : 0);
    vol.ghostGridOrigin = gridOrigin - vec3f(ghostOffset) * gridSpacing;

    vol.volume = ospray::cpp::Volume("block_bricked_volume");
    vol.volume.set("voxelType", "uchar");
    vol.volume.set("dimensions", fullDims);
    vol.volume.set("transferFunction", vol.tfcn);
    vol.volume.set("gridSpacing", gridSpacing);
    vol.volume.set("gridOrigin", vol.ghostGridOrigin);

    std::vector<unsigned char> volumeData(fullDims.x * fullDims.y * fullDims.z, 0);
    for (size_t i = 0; i < volumeData.size(); ++i) {
      volumeData[i] = myRank;
    }
    vol.volume.setRegion(volumeData.data(), vec3i(0), fullDims);
    vol.volume.commit();

    vol.bounds = box3f(gridOrigin, gridOrigin + vec3f(1.f) / vec3f(grid));
    return vol;
  }

  size_t sizeForDtype(const std::string &dtype)
  {
    if (dtype == "uchar" || dtype == "char") {
      return 1;
    }
    if (dtype == "float") {
      return 4;
    }
    if (dtype == "double") {
      return 8;
    }

    return 0;
  }

  LoadedVolume loadVolume(const FileName &file, const vec3i &dimensions,
                          const std::string &dtype, const vec2f &valueRange)
  {
    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

    LoadedVolume vol;
    vol.tfcn.set("valueRange", valueRange);
    vol.tfcn.commit();

    const vec3sz grid = vec3sz(computeGrid(numRanks));
    const vec3sz brickDims = vec3sz(dimensions) / grid;
    const vec3sz brickId(myRank % grid.x, (myRank / grid.x) % grid.y, myRank / (grid.x * grid.y));
    const vec3f gridOrigin = vec3f(brickId) * vec3f(brickDims);

    const std::array<int, 3> ghosts = computeGhostFaces(vec3i(brickId), vec3i(grid));
    vec3sz ghostDims(0);
    for (size_t i = 0; i < 3; ++i) {
      if (ghosts[i] & POS_FACE) {
        ghostDims[i] += 1;
      }
      if (ghosts[i] & NEG_FACE) {
        ghostDims[i] += 1;
      }
    }
    const vec3sz fullDims = brickDims + ghostDims;
    const vec3i ghostOffset(ghosts[0] & NEG_FACE ? 1 : 0,
                               ghosts[1] & NEG_FACE ? 1 : 0,
                               ghosts[2] & NEG_FACE ? 1 : 0);
    vol.ghostGridOrigin = gridOrigin - vec3f(ghostOffset);

    vol.volume = ospray::cpp::Volume("block_bricked_volume");
    vol.volume.set("voxelType", dtype.c_str());
    vol.volume.set("dimensions", vec3i(fullDims));
    vol.volume.set("transferFunction", vol.tfcn);
    vol.volume.set("gridOrigin", vol.ghostGridOrigin);

    const size_t dtypeSize = sizeForDtype(dtype);
    std::vector<unsigned char> volumeData(fullDims.x * fullDims.y * fullDims.z * dtypeSize, 0);

    RawReader reader(file, vec3sz(dimensions), dtypeSize);
    reader.readRegion(brickId * brickDims - vec3sz(ghostOffset),
        vec3sz(fullDims), volumeData.data());
    vol.volume.setRegion(volumeData.data(), vec3i(0), vec3i(fullDims));
    vol.volume.commit();

    vol.bounds = box3f(gridOrigin, gridOrigin + vec3f(brickDims));
    return vol;
  }

} // ::gensv

