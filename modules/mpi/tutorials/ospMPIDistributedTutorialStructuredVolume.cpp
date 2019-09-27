// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

/* This larger example shows how to use the MPIDistributedDevice to write an
 * interactive rendering application, which shows a UI on rank 0 and uses
 * all ranks in the MPI world for data loading and rendering. Each rank
 * generates a local sub-brick of volume data, as if rendering some
 * large distributed dataset.
 *
 * Each local brick of data is put into a model, which is given a unique
 * id, to identify it as a piece of data owned uniquely by the process
 */

#include <iterator>
#include <memory>
#include <random>

#include <mpi.h>
#include "GLFWDistribOSPRayWindow.h"

#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;
using namespace ospcommon::math;

struct VolumeBrick
{
  // the volume data itself
  OSPVolume brick;
  OSPVolumetricModel model;
  OSPGroup group;
  OSPInstance instance;
  // the bounds of the owned portion of data
  box3f bounds;
  // the full bounds of the owned portion + ghost voxels
  box3f ghostBounds;
};

static box3f worldBounds;

// Generate the rank's local volume brick
VolumeBrick makeLocalVolume(const int mpiRank, const int mpiWorldSize);

int main(int argc, char **argv)
{
  int mpiThreadCapability = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpiThreadCapability);
  if (mpiThreadCapability != MPI_THREAD_MULTIPLE
      && mpiThreadCapability != MPI_THREAD_SERIALIZED) {
    fprintf(stderr,
        "OSPRay requires the MPI runtime to support thread "
        "multiple or thread serialized.\n");
    return 1;
  }

  int mpiRank = 0;
  int mpiWorldSize = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  std::cout << "OSPRay rank " << mpiRank << "/" << mpiWorldSize << "\n";

  // load the MPI module, and select the MPI distributed device. Here we
  // do not call ospInit, as we want to explicitly pick the distributed
  // device. This can also be done by passing --osp:mpi-distributed when
  // using ospInit, however if the user doesn't pass this argument your
  // application will likely not behave as expected
  ospLoadModule("mpi");

  OSPDevice mpiDevice = ospNewDevice("mpi_distributed");
  ospDeviceCommit(mpiDevice);
  ospSetCurrentDevice(mpiDevice);

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // all ranks specify the same rendering parameters, with the exception of
  // the data to be rendered, which is distributed among the ranks
  VolumeBrick brick = makeLocalVolume(mpiRank, mpiWorldSize);

  // color the bricks by their rank, we pad the range out a bit to keep
  // any brick from being completely transparent
  OSPTransferFunction tfn = ospTestingNewTransferFunction(
      osp_vec2f{-1.0f, static_cast<float>(mpiWorldSize) + 1}, "jet");
  ospSetObject(brick.model, "transferFunction", tfn);
  ospSetFloat(brick.model, "samplingRate", 0.5f);
  ospCommit(brick.model);

  // create the "world" model which will contain all of our geometries
  OSPWorld world = ospNewWorld();
  OSPData instances = ospNewData(1, OSP_INSTANCE, &brick.instance);
  ospSetData(world, "instance", instances);
  ospRelease(instances);

  OSPData regionData = ospNewData(1, OSP_BOX3F, &brick.bounds, 0);
  ospCommit(regionData);
  ospSetObject(world, "regions", regionData);
  ospRelease(regionData);

  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("mpi_raycast");

  // create and setup an ambient light
  OSPLight ambientLight = ospNewLight("ambient");
  ospCommit(ambientLight);
  OSPData lightData = ospNewData(1, OSP_LIGHT, &ambientLight, 0);
  ospCommit(lightData);
  ospSetObject(renderer, "light", lightData);
  ospRelease(lightData);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
          vec2i{1024, 768}, worldBounds, world, renderer));

  int spp = 1;
  int currentSpp = 1;
  if (mpiRank == 0) {
    glfwOSPRayWindow->registerImGuiCallback(
        [&]() { ImGui::SliderInt("spp", &spp, 1, 64); });
  }

  glfwOSPRayWindow->registerDisplayCallback([&](GLFWDistribOSPRayWindow *win) {
    // Send the UI changes out to the other ranks so we can synchronize
    // how many samples per-pixel we're taking
    MPI_Bcast(&spp, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (spp != currentSpp) {
      currentSpp = spp;
      ospSetInt(renderer, "spp", spp);
      win->addObjectToCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  MPI_Finalize();

  return 0;
}

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

// Compute an X x Y x Z grid to have 'num' grid cells,
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

VolumeBrick makeLocalVolume(const int mpiRank, const int mpiWorldSize)
{
  const vec3i grid = computeGrid(mpiWorldSize);
  const vec3i brickId(mpiRank % grid.x,
      (mpiRank / grid.x) % grid.y,
      mpiRank / (grid.x * grid.y));
  // The bricks are 64^3 + 1 layer of ghost voxels on each axis
  const vec3i brickVolumeDims = vec3i(32);
  const vec3i brickGhostDims = vec3i(brickVolumeDims + 2);

  // The grid is over the [0, grid * brickVolumeDims] box
  worldBounds = box3f(vec3f(0.f), vec3f(grid * brickVolumeDims));
  const vec3f brickLower = brickId * brickVolumeDims;
  const vec3f brickUpper = brickId * brickVolumeDims + brickVolumeDims;

  VolumeBrick brick;
  brick.bounds = box3f(brickLower, brickUpper);
  // we just put ghost voxels on all sides here, but a real application
  // would change which faces of each brick have ghost voxels dependent
  // on the actual data
  brick.ghostBounds = box3f(brickLower - vec3f(1.f), brickUpper + vec3f(1.f));

  brick.brick = ospNewVolume("shared_structured_volume");

  ospSetInt(brick.brick, "voxelType", OSP_UCHAR);
  ospSetParam(brick.brick, "dimensions", OSP_VEC3I, &brickGhostDims.x);
  // we use the grid origin to place this brick in the right position inside
  // the global volume
  ospSetParam(brick.brick, "gridOrigin", OSP_VEC3F, &brick.ghostBounds.lower.x);

  // generate the volume data to just be filled with this rank's id
  const size_t nVoxels = brickGhostDims.x * brickGhostDims.y * brickGhostDims.z;
  std::vector<char> volumeData(nVoxels, static_cast<char>(mpiRank));
  OSPData ospVolumeData =
      ospNewData(volumeData.size(), OSP_UCHAR, volumeData.data());
  ospSetObject(brick.brick, "voxelData", ospVolumeData);

  // Set the clipping box of the volume to clip off the ghost voxels
  ospSetParam(
      brick.brick, "volumeClippingBoxLower", OSP_VEC3F, &brick.bounds.lower.x);
  ospSetParam(
      brick.brick, "volumeClippingBoxUpper", OSP_VEC3F, &brick.bounds.upper.x);
  ospCommit(brick.brick);

  brick.model = ospNewVolumetricModel(brick.brick);

  brick.group = ospNewGroup();
  OSPData volumes = ospNewData(1, OSP_VOLUMETRIC_MODEL, &brick.model);
  ospSetObject(brick.group, "volume", volumes);
  ospCommit(brick.group);
  ospRelease(volumes);

  brick.instance = ospNewInstance(brick.group);
  ospCommit(brick.instance);

  return brick;
}
