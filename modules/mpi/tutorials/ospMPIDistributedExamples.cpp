// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/* This example generates the same data on each rank so that we can build
 * off the existing OSPRay testing data generators, however each rank
 * specifies a subregion as its local domain to render the data as if it was
 * distributed. The scene to load can be passed as an argument to the
 * application
 */

#include <imgui.h>
#include <mpi.h>
#include <iterator>
#include <memory>
#include <random>
#include "GLFWDistribOSPRayWindow.h"
#include "ospray/ospray_cpp.h"
#include "ospray/ospray_cpp/ext/rkcommon.h"
#include "ospray/ospray_util.h"
#include "ospray_testing.h"

using namespace ospray;
using namespace rkcommon;
using namespace rkcommon::math;

bool computeDivisor(int x, int &divisor);

// Compute an X x Y x Z grid to have 'num' grid cells,
// only gives a nice grid for numbers with even factors since
// we don't search for factors of the number, we just try dividing by two
vec3i computeGrid(int num);

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

  std::string sceneName = "gravity_spheres_volume";
  if (argc == 2) {
    sceneName = argv[1];
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
    cpp::Device mpiDevice("mpiDistributed");
    mpiDevice.commit();
    mpiDevice.setCurrent();

    // set an error callback to catch any OSPRay errors and exit the application
    mpiDevice.setErrorCallback(
        [](void *, OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          exit(error);
        });

    // Regions are assigned to ranks in 3D grid, determine which brick of the
    // grid is owned by this rank
    const vec3i distribGrid = computeGrid(mpiWorldSize);
    const vec3i distribBrickId(mpiRank % distribGrid.x,
        (mpiRank / distribGrid.x) % distribGrid.y,
        mpiRank / (distribGrid.x * distribGrid.y));

    std::cout << "scene = " << sceneName << "\n";
    auto builder = testing::newBuilder(sceneName);
    testing::setParam(builder, "rendererType", "scivis");
    testing::commit(builder);

    auto group = testing::buildGroup(builder);
    const box3f worldBounds = group.getBounds<box3f>();

    cpp::Instance instance(group);
    instance.commit();

    cpp::World world;
    world.setParam("instance", cpp::CopiedData(instance));

    cpp::Light light("ambient");
    light.setParam("visible", false);
    light.commit();

    world.setParam("light", cpp::CopiedData(light));

    // Determine the bounds of this rank's region in world space
    const vec3f distribBrickDims = worldBounds.size() / vec3f(distribGrid);
    box3f localRegion(distribBrickId * distribBrickDims + worldBounds.lower,
        distribBrickId * distribBrickDims + distribBrickDims
            + worldBounds.lower);

    // Special case for the ospray test data: we might have geometry right at
    // the region bounding box which will z-fight with the clipping region. If
    // we have a region at the edge of the domain, apply some padding
    for (int i = 0; i < 3; ++i) {
      if (localRegion.lower[i] == worldBounds.lower[i]) {
        localRegion.lower[i] -= 0.001;
      }
      if (localRegion.upper[i] == worldBounds.upper[i]) {
        localRegion.upper[i] += 0.001;
      }
    }

    // Set our region that represents the bounds of the local data we own on
    // this rank
    world.setParam("region", cpp::CopiedData(localRegion));
    world.commit();
    testing::release(builder);

    // create OSPRay renderer
    cpp::Renderer renderer("mpiRaycast");

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow =
        std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
            vec2i{1024, 768}, worldBounds, world, renderer));

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
