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
 * all ranks in the MPI world for data loading and rendering. This example
 * also shows how to leverage the support for partially replicated data
 * distributions in the MPIDistributedDevice, by sharing bricks of data,
 * and thus rendering work, between processes.
 *
 * Each shared brick of data is put into a model, and is given a unique
 * the same id on the processes which share it, to identify the model as
 * being shared across the set of processes. In this example every two proceses
 * will share data, so 0 & 1 will share data, 2 & 3, and so on.
 */

#include <imgui.h>
#include <mpi.h>
#include <ospray/ospray_cpp.h>
#include <ospray/ospray_util.h>
#include <iterator>
#include <memory>
#include <random>
#include "GLFWDistribOSPRayWindow.h"
#include "ospray_testing.h"

using namespace ospray;
using namespace ospcommon;
using namespace ospcommon::math;

struct VolumeBrick
{
  // the volume data itself
  cpp::Volume brick;
  cpp::VolumetricModel model;
  cpp::Group group;
  cpp::Instance instance;
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

  {
    cpp::Device mpiDevice("mpi_distributed");
    mpiDevice.commit();
    mpiDevice.setCurrent();

    // set an error callback to catch any OSPRay errors and exit the application
    ospDeviceSetErrorFunc(
        ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          exit(error);
        });

    // every two ranks will share a volume brick to render, if we have an
    // odd number of ranks the last one will have its own brick
    const int sharedWorldSize = mpiWorldSize / 2 + mpiWorldSize % 2;

    // all ranks specify the same rendering parameters, with the exception of
    // the data to be rendered, which is distributed among the ranks
    VolumeBrick brick = makeLocalVolume(mpiRank / 2, sharedWorldSize);

    // create the "world" model which will contain all of our geometries
    cpp::World world;
    world.setParam("instance", cpp::Data(brick.instance));

    world.setParam("regions", cpp::Data(brick.bounds));
    world.commit();

    // create OSPRay renderer
    cpp::Renderer renderer("mpi_raycast");

    // create and setup an ambient light
    cpp::Light ambientLight("ambient");
    ambientLight.commit();
    renderer.setParam("light", cpp::Data(ambientLight));

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow =
        std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
            vec2i{1024, 768}, worldBounds, world.handle(), renderer.handle()));

    int spp = 1;
    int currentSpp = 1;
    if (mpiRank == 0) {
      glfwOSPRayWindow->registerImGuiCallback(
          [&]() { ImGui::SliderInt("pixelSamples", &spp, 1, 64); });
    }

    glfwOSPRayWindow->registerDisplayCallback(
        [&](GLFWDistribOSPRayWindow *win) {
          // Send the UI changes out to the other ranks so we can synchronize
          // how many samples per-pixel we're taking
          MPI_Bcast(&spp, 1, MPI_INT, 0, MPI_COMM_WORLD);
          if (spp != currentSpp) {
            currentSpp = spp;
            renderer.setParam("pixelSamples", spp);
            win->addObjectToCommit(renderer.handle());
          }
        });

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();
  }

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

  brick.brick = cpp::Volume("structuredRegular");

  brick.brick.setParam("dimensions", brickGhostDims);

  // we use the grid origin to place this brick in the right position inside
  // the global volume
  brick.brick.setParam("gridOrigin", brick.ghostBounds.lower);

  // generate the volume data to just be filled with this rank's id
  const size_t nVoxels = brickGhostDims.x * brickGhostDims.y * brickGhostDims.z;
  std::vector<uint8_t> volumeData(nVoxels, static_cast<uint8_t>(mpiRank));
  brick.brick.setParam("voxelData", cpp::Data(volumeData));

  // Set the clipping box of the volume to clip off the ghost voxels
  brick.brick.setParam("volumeClippingBoxLower", brick.bounds.lower);
  brick.brick.setParam("volumeClippingBoxUpper", brick.bounds.upper);
  brick.brick.commit();

  brick.model = cpp::VolumetricModel(brick.brick);
  cpp::TransferFunction tfn("piecewise_linear");
  std::vector<vec3f> colors = {vec3f(0.f, 0.f, 1.f), vec3f(1.f, 0.f, 0.f)};
  std::vector<float> opacities = {0.05f, 1.f};

  tfn.setParam("color", cpp::Data(colors));
  tfn.setParam("opacity", cpp::Data(opacities));
  // color the bricks by their rank, we pad the range out a bit to keep
  // any brick from being completely transparent
  vec2f valueRange = vec2f(0, mpiWorldSize);
  tfn.setParam("valueRange", valueRange);
  tfn.commit();
  brick.model.setParam("transferFunction", tfn);
  brick.model.setParam("samplingRate", 0.5f);
  brick.model.commit();

  brick.group = cpp::Group();
  brick.group.setParam("volume", cpp::Data(brick.model));
  brick.group.commit();

  brick.instance = cpp::Instance(brick.group);
  brick.instance.commit();

  return brick;
}
