// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/* This tutorial demonstrates how MPI-parallel applications can be
 * written which still render replicated data using the path tracer
 * for secondary effects. Applications can leverage MPI parallelism for
 * faster file I/O and load times when compared to the offload device.
 * However, such applications must be written to be aware of MPI while
 * the offload device behaves just like local rendering from the application's
 * perspective.
 */

#include <imgui.h>
#include <mpi.h>
#include <ospray/ospray_util.h>
#include <iterator>
#include <memory>
#include <random>
#include "GLFWDistribOSPRayWindow.h"
#include "example_common.h"
#include "ospray_testing.h"

using namespace ospray;
using namespace rkcommon;
using namespace rkcommon::math;

static std::string rendererType = "pathtracer";
static std::string builderType = "perlin_noise_volumes";

void printHelp()
{
  std::cout <<
      R"description(
usage: ./ospExamples [-h | --help] [[-s | --scene] scene] [[r | --renderer] renderer_type]

scenes:

  boxes
  cornell_box
  curves
  cylinders
  empty
  gravity_spheres_volume
  perlin_noise_volumes
  random_spheres
  streamlines
  subdivision_cube
  unstructured_volume

  )description";
}

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printHelp();
      return 0;
    } else if (arg == "-r" || arg == "--renderer") {
      rendererType = argv[++i];
    } else if (arg == "-s" || arg == "--scene") {
      builderType = argv[++i];
    }
  }

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
    cpp::Device mpiDevice("mpiDistributed");
    mpiDevice.commit();
    mpiDevice.setCurrent();

    // set an error callback to catch any OSPRay errors and exit the application
    ospDeviceSetErrorFunc(
        mpiDevice.handle(), [](OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          exit(error);
        });

    auto builder = testing::newBuilder(builderType);
    testing::setParam(builder, "rendererType", rendererType);
    testing::commit(builder);

    auto world = testing::buildWorld(builder);
    testing::release(builder);

    world.commit();

    cpp::Renderer renderer(rendererType);
    renderer.commit();

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow = std::unique_ptr<GLFWDistribOSPRayWindow>(
        new GLFWDistribOSPRayWindow(vec2i{1024, 768},
            box3f(vec3f(-1.f), vec3f(1.f)),
            world.handle(),
            renderer.handle()));

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();
  }
  // cleanly shut OSPRay down
  ospShutdown();

  MPI_Finalize();

  return 0;
}
