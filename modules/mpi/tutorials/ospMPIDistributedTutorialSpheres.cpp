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
 * generates a local sub-piece of spheres data, e.g., as if rendering some
 * large distributed dataset.
 *
 * Each local brick of data is put into a model, which is given a unique
 * id, to identify it as a piece of data owned uniquely by the process
 */

#include <imgui.h>
#include <mpi.h>
#include <array>
#include <iterator>
#include <memory>
#include <random>
#include "GLFWDistribOSPRayWindow.h"
#include "ospray_testing.h"

using namespace ospcommon;
using namespace ospcommon::math;

// Generate the rank's local spheres within its assigned grid cell, and
// return the bounds of this grid cell
OSPInstance makeLocalSpheres(
    const int mpiRank, const int mpiWorldSize, box3f &bounds);

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
  box3f regionBounds;
  OSPInstance spheres = makeLocalSpheres(mpiRank, mpiWorldSize, regionBounds);

  // create the "world" model which will contain all of our geometries
  OSPWorld world = ospNewWorld();
  OSPData geometryInstances = ospNewSharedData(&spheres, OSP_INSTANCE, 1);
  ospSetObject(world, "instance", geometryInstances);
  ospRelease(spheres);
  ospRelease(geometryInstances);

  /*
   * Note: We've taken care that all the generated spheres are completely
   * within the bounds, and we don't have ghost data or portions of speres
   * to clip off. Thus we actually don't need to set regions at all in
   * this tutorial
  OSPData regionData = ospNewSharedData(&regionBounds, OSP_BOX3F, 1);
  ospCommit(regionData);
  ospSetObject(world, "regions", regionData);
  ospRelease(regionData);
  */

  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("mpi_raycast");

  // create and setup an ambient light
  std::array<OSPLight, 2> lights = {
      ospNewLight("ambient"), ospNewLight("distant")};
  ospCommit(lights[0]);

  ospSetVec3f(lights[1], "direction", -1.f, -1.f, 0.5f);
  ospCommit(lights[1]);

  OSPData lightData = ospNewSharedData(lights.data(), OSP_LIGHT, lights.size());
  ospCommit(lightData);
  // TODO: Who takes the lights params?
  ospSetObject(renderer, "lights", lightData);
  ospSetInt(renderer, "aoSamples", 1);
  ospRelease(lightData);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

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

OSPInstance makeLocalSpheres(
    const int mpiRank, const int mpiWorldSize, box3f &bounds)
{
  const float sphereRadius = 0.1;
  std::vector<vec3f> spheres(50);

  // To simulate loading a shared dataset all ranks generate the same
  // sphere data.
  std::random_device rd;
  std::mt19937 rng(rd());

  const vec3i grid = computeGrid(mpiWorldSize);
  const vec3i brickId(mpiRank % grid.x,
      (mpiRank / grid.x) % grid.y,
      mpiRank / (grid.x * grid.y));

  // The grid is over the [-1, 1] box
  const vec3f brickSize = vec3f(2.0) / vec3f(grid);
  const vec3f brickLower = brickSize * brickId - vec3f(1.f);
  const vec3f brickUpper = brickSize * brickId - vec3f(1.f) + brickSize;

  bounds.lower = brickLower;
  bounds.upper = brickUpper;

  // Generate spheres within the box padded by the radius, so we don't need
  // to worry about ghost bounds
  std::uniform_real_distribution<float> distX(
      brickLower.x + sphereRadius, brickUpper.x - sphereRadius);
  std::uniform_real_distribution<float> distY(
      brickLower.y + sphereRadius, brickUpper.y - sphereRadius);
  std::uniform_real_distribution<float> distZ(
      brickLower.z + sphereRadius, brickUpper.z - sphereRadius);

  for (auto &s : spheres) {
    s.x = distX(rng);
    s.y = distY(rng);
    s.z = distZ(rng);
  }

  OSPData sphereData = ospNewSharedData(spheres.data(), OSP_VEC3F, spheres.size());
  ospCommit(sphereData);

  vec3f color(0.f, 0.f, (mpiRank + 1.f) / mpiWorldSize);
  OSPMaterial material = ospNewMaterial("scivis", "SciVisMaterial");
  ospSetParam(material, "Kd", OSP_VEC3F, &color.x);
  ospCommit(material);

  OSPGeometry sphereGeom = ospNewGeometry("spheres");
  ospSetFloat(sphereGeom, "radius", sphereRadius);
  ospSetObject(sphereGeom, "sphere.position", sphereData);
  ospCommit(sphereGeom);

  OSPGeometricModel model = ospNewGeometricModel(sphereGeom);
  ospSetObject(model, "material", material);
  ospCommit(model);

  OSPGroup group = ospNewGroup();
  auto models = ospNewSharedData(&model, OSP_GEOMETRIC_MODEL, 1);
  ospSetObject(group, "geometry", models);
  ospCommit(group);
  ospRelease(models);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);

  ospRelease(material);
  ospRelease(sphereData);
  ospRelease(sphereGeom);

  return instance;
}
