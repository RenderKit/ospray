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

#include <iterator>
#include <memory>
#include <random>
#include <mpi.h>
#include "GLFWDistribOSPRayWindow.h"

#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;
using namespace ospcommon::math;

static std::string renderer_type = "pathtracer";

// NOTE: We use our own here because both ranks need to have the same random
// seed used when generating the spheres
OSPTestingGeometry createSpheres(int mpiRank);
OSPInstance createGroundPlane(std::string renderer_type,
                              float planeExtent = 1.5f);

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  int mpiThreadCapability = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpiThreadCapability);
  if (mpiThreadCapability != MPI_THREAD_MULTIPLE &&
      mpiThreadCapability != MPI_THREAD_SERIALIZED) {
    fprintf(stderr,
            "OSPRay requires the MPI runtime to support thread "
            "multiple or thread serialized.\n");
    return 1;
  }

  int mpiRank      = 0;
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
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());

  ospSetData(world, "instance", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "light", lightsData);
  ospRelease(lightsData);

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

OSPTestingGeometry createSpheres(int mpiRank) {
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
  std::vector<Sphere> spheres(numSpheres);
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
  OSPData spheresData =
    ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, spheres.data());

  // create the sphere geometry, and assign attributes
  OSPGeometry spheresGeometry = ospNewGeometry("spheres");

  ospSetData(spheresGeometry, "sphere", spheresData);
  ospSetInt(spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
  ospSetInt(
      spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
  ospSetInt(
      spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

  // commit the spheres geometry
  ospCommit(spheresGeometry);

  OSPGeometricModel model = ospNewGeometricModel(spheresGeometry);

  OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, colors.data());

  ospSetData(model, "prim.color", colorData);

  // create glass material and assign to geometry
  OSPMaterial glassMaterial =
    ospNewMaterial(renderer_type.c_str(), "ThinGlass");
  ospSetFloat(glassMaterial, "attenuationDistance", 0.2f);
  ospCommit(glassMaterial);

  ospSetObject(model, "material", glassMaterial);

  // release handles we no longer need
  ospRelease(spheresData);
  ospRelease(colorData);
  ospRelease(glassMaterial);

  ospCommit(model);

  OSPGroup group = ospNewGroup();
  auto models    = ospNewData(1, OSP_OBJECT, &model);
  ospSetData(group, "geometry", models);
  ospCommit(group);
  ospRelease(models);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);

  OSPTestingGeometry retval;
  retval.geometry = spheresGeometry;
  retval.model    = model;
  retval.group    = group;
  retval.instance = instance;
  retval.bounds   = reinterpret_cast<osp_box3f &>(bounds);

  return retval;
}

OSPInstance createGroundPlane(std::string renderer_type, float planeExtent)
{
  OSPGeometry planeGeometry = ospNewGeometry("quads");

  struct Vertex
  {
    vec3f position;
    vec3f normal;
    vec4f color;
  };

  struct QuadIndex
  {
    int x;
    int y;
    int z;
    int w;
  };

  std::vector<Vertex> vertices;
  std::vector<QuadIndex> quadIndices;

  // ground plane
  int startingIndex = vertices.size();

  const vec3f up   = vec3f{0.f, 1.f, 0.f};
  const vec4f gray = vec4f{0.9f, 0.9f, 0.9f, 0.75f};

  vertices.push_back(Vertex{vec3f{-planeExtent, -1.f, -planeExtent}, up, gray});
  vertices.push_back(Vertex{vec3f{planeExtent, -1.f, -planeExtent}, up, gray});
  vertices.push_back(Vertex{vec3f{planeExtent, -1.f, planeExtent}, up, gray});
  vertices.push_back(Vertex{vec3f{-planeExtent, -1.f, planeExtent}, up, gray});

  quadIndices.push_back(QuadIndex{
      startingIndex, startingIndex + 1, startingIndex + 2, startingIndex + 3});

  // stripes on ground plane
  const float stripeWidth  = 0.025f;
  const float paddedExtent = planeExtent + stripeWidth;
  const size_t numStripes  = 10;

  const vec4f stripeColor = vec4f{1.0f, 0.1f, 0.1f, 1.f};

  for (size_t i = 0; i < numStripes; i++) {
    // the center coordinate of the stripe, either in the x or z direction
    const float coord =
        -planeExtent + float(i) / float(numStripes - 1) * 2.f * planeExtent;

    // offset the stripes by an epsilon above the ground plane
    const float yLevel = -1.f + 1e-3f;

    // x-direction stripes
    startingIndex = vertices.size();

    vertices.push_back(Vertex{
        vec3f{-paddedExtent, yLevel, coord - stripeWidth}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{paddedExtent, yLevel, coord - stripeWidth}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{paddedExtent, yLevel, coord + stripeWidth}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{-paddedExtent, yLevel, coord + stripeWidth}, up, stripeColor});

    quadIndices.push_back(QuadIndex{startingIndex,
                                    startingIndex + 1,
                                    startingIndex + 2,
                                    startingIndex + 3});

    // z-direction stripes
    startingIndex = vertices.size();

    vertices.push_back(Vertex{
        vec3f{coord - stripeWidth, yLevel, -paddedExtent}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{coord + stripeWidth, yLevel, -paddedExtent}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{coord + stripeWidth, yLevel, paddedExtent}, up, stripeColor});
    vertices.push_back(Vertex{
        vec3f{coord - stripeWidth, yLevel, paddedExtent}, up, stripeColor});

    quadIndices.push_back(QuadIndex{startingIndex,
                                    startingIndex + 1,
                                    startingIndex + 2,
                                    startingIndex + 3});
  }

  // create OSPRay data objects
  std::vector<vec3f> positionVector;
  std::vector<vec3f> normalVector;
  std::vector<vec4f> colorVector;

  std::transform(vertices.begin(),
                 vertices.end(),
                 std::back_inserter(positionVector),
                 [](Vertex const &v) { return v.position; });
  std::transform(vertices.begin(),
                 vertices.end(),
                 std::back_inserter(normalVector),
                 [](Vertex const &v) { return v.normal; });
  std::transform(vertices.begin(),
                 vertices.end(),
                 std::back_inserter(colorVector),
                 [](Vertex const &v) { return v.color; });

  OSPData positionData =
      ospNewData(vertices.size(), OSP_VEC3F, positionVector.data());
  OSPData normalData =
      ospNewData(vertices.size(), OSP_VEC3F, normalVector.data());
  OSPData colorData =
      ospNewData(vertices.size(), OSP_VEC4F, colorVector.data());
  OSPData indexData =
      ospNewData(quadIndices.size(), OSP_VEC4I, quadIndices.data());

  // set vertex / index data on the geometry
  ospSetData(planeGeometry, "vertex.position", positionData);
  ospSetData(planeGeometry, "vertex.normal", normalData);
  ospSetData(planeGeometry, "vertex.color", colorData);
  ospSetData(planeGeometry, "index", indexData);

  // finally, commit the geometry
  ospCommit(planeGeometry);

  OSPGeometricModel model = ospNewGeometricModel(planeGeometry);

  ospRelease(planeGeometry);

  // create and assign a material to the geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospCommit(material);

  ospSetObject(model, "material", material);

  ospCommit(model);

  OSPGroup group = ospNewGroup();

  OSPData models = ospNewData(1, OSP_OBJECT, &model);
  ospSetData(group, "geometry", models);
  ospCommit(group);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // release handles we no longer need
  ospRelease(positionData);
  ospRelease(normalData);
  ospRelease(colorData);
  ospRelease(indexData);
  ospRelease(material);
  ospRelease(model);
  ospRelease(models);

  return instance;
}
