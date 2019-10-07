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
#include "ospray_testing.h"
#include "tutorial_util.h"

using namespace ospcommon;
using namespace ospcommon::math;

static std::string renderer_type = "pathtracer";

// NOTE: We use our own here because both ranks need to have the same random
// seed used when generating the spheres
OSPTestingGeometry createSpheres(int mpiRank);

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
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

  OSPDevice mpiDevice = ospNewDevice("mpi_distributed");
  ospDeviceCommit(mpiDevice);
  ospSetCurrentDevice(mpiDevice);

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  std::vector<OSPInstance> instanceHandles;

  // add in spheres geometry
  OSPTestingGeometry spheres = createSpheres(mpiRank);
  instanceHandles.push_back(spheres.instance);
  ospRelease(spheres.geometry);

  // add in a ground plane geometry
  OSPInstance planeInstance = createGroundPlane(renderer_type);
  ospCommit(planeInstance);
  instanceHandles.push_back(planeInstance);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_INSTANCE, instanceHandles.data());

  ospSetObject(world, "instance", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetObject(world, "light", lightsData);
  ospRelease(lightsData);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());
  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  MPI_Finalize();

  return 0;
}

OSPTestingGeometry createSpheres(int mpiRank)
{
  struct Sphere
  {
    vec3f center;
    float radius;
  };

  const int numSpheres = 100;

  // create random number distributions for sphere center, radius, and color
  int seed = 0;
  if (mpiRank == 0) {
    std::random_device rd;
    seed = rd();
    MPI_Bcast(&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
  } else {
    MPI_Bcast(&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }
  std::mt19937 gen(seed);

  std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
  std::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);
  std::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

  // populate the spheres
  box3f bounds;
  static std::vector<Sphere> spheres(numSpheres);
  std::vector<vec4f> colors(numSpheres);

  for (auto &s : spheres) {
    s.center.x = centerDistribution(gen);
    s.center.y = centerDistribution(gen);
    s.center.z = centerDistribution(gen);

    s.radius = radiusDistribution(gen);

    bounds.extend(box3f(s.center - s.radius, s.center + s.radius));
  }

  for (auto &c : colors) {
    c.x = colorDistribution(gen);
    c.y = colorDistribution(gen);
    c.z = colorDistribution(gen);
    c.w = colorDistribution(gen);
  }

  // create a data object with all the sphere information
  OSPData positionData =
      ospNewSharedData((char *)spheres.data() + offsetof(Sphere, center),
          OSP_VEC3F,
          numSpheres,
          sizeof(Sphere));
  OSPData radiusData =
      ospNewSharedData((char *)spheres.data() + offsetof(Sphere, radius),
          OSP_FLOAT,
          numSpheres,
          sizeof(Sphere));

  // create the sphere geometry, and assign attributes
  OSPGeometry spheresGeometry = ospNewGeometry("spheres");

  ospSetObject(spheresGeometry, "sphere.position", positionData);
  ospSetObject(spheresGeometry, "sphere.radius", radiusData);

  // commit the spheres geometry
  ospCommit(spheresGeometry);

  OSPGeometricModel model = ospNewGeometricModel(spheresGeometry);

  OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, colors.data());

  ospSetObject(model, "color", colorData);

  // create glass material and assign to geometry
  OSPMaterial glassMaterial =
      ospNewMaterial(renderer_type.c_str(), "ThinGlass");
  ospSetFloat(glassMaterial, "attenuationDistance", 0.2f);
  ospCommit(glassMaterial);

  ospSetObjectAsData(model, "material", OSP_MATERIAL, glassMaterial);

  ospCommit(model);

  // release handles we no longer need
  ospRelease(positionData);
  ospRelease(radiusData);
  ospRelease(colorData);
  ospRelease(glassMaterial);

  OSPGroup group = ospNewGroup();
  ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, model);
  ospCommit(group);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);

  OSPTestingGeometry retval;
  retval.geometry = spheresGeometry;
  retval.model = model;
  retval.group = group;
  retval.instance = instance;

  std::memcpy(&retval.bounds, &bounds, sizeof(bounds));
  return retval;
}
