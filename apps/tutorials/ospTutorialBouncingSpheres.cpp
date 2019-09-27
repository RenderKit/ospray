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

#include <iterator>
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include "ospray_testing.h"

#include "tutorial_util.h"

#include <imgui.h>

static std::string renderer_type = "pathtracer";

using namespace ospcommon;

struct Sphere
{
  // initial (maximum) height of the sphere, at which it has 0 velocity
  float maxHeight;

  vec3f center;
  float radius;
};

// track the spheres data, geometry, and world globally so we can update them
// every frame
static std::vector<Sphere> g_spheres;
static std::vector<vec4f> g_colors;
static OSPData g_positionData;
static OSPGeometry g_spheresGeometry;
static OSPGeometricModel g_spheresModel;
static OSPGroup g_spheresGroup;
static OSPWorld g_world;

std::vector<Sphere> generateRandomSpheres(size_t numSpheres)
{
  // create random number distributions for sphere center, radius, and color
  std::random_device rd;
  std::mt19937 gen(rd());

  std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
  std::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);
  std::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

  // populate the spheres
  std::vector<Sphere> spheres(numSpheres);

  g_colors.resize(numSpheres);

  for (auto &s : spheres) {
    // maximum height above y=-1 ground plane
    s.maxHeight = 1.f + centerDistribution(gen);

    s.center.x = centerDistribution(gen);
    s.center.y = s.maxHeight;
    s.center.z = centerDistribution(gen);

    s.radius = radiusDistribution(gen);
  }

  for (auto &c : g_colors) {
    c.x = colorDistribution(gen);
    c.y = colorDistribution(gen);
    c.z = colorDistribution(gen);
    c.w = colorDistribution(gen);
  }

  return spheres;
}

OSPGeometricModel createRandomSpheresGeometry(size_t numSpheres)
{
  g_spheres = generateRandomSpheres(numSpheres);

  // create a data objects with all the sphere information
  g_positionData =
      ospNewSharedData((char *)g_spheres.data() + offsetof(Sphere, center),
          OSP_VEC3F,
          numSpheres,
          sizeof(Sphere));
  OSPData radiusData =
      ospNewSharedData((char *)g_spheres.data() + offsetof(Sphere, radius),
          OSP_FLOAT,
          numSpheres,
          sizeof(Sphere));

  // create the sphere geometry, and assign attributes
  g_spheresGeometry = ospNewGeometry("spheres");
  g_spheresModel    = ospNewGeometricModel(g_spheresGeometry);

  ospSetObject(g_spheresGeometry, "sphere.position", g_positionData);
  ospSetObject(g_spheresGeometry, "sphere.radius", radiusData);

  OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, g_colors.data());

  ospSetObject(g_spheresModel, "prim.color", colorData);

  // create glass material and assign to geometry
  OSPMaterial glassMaterial =
      ospNewMaterial(renderer_type.c_str(), "ThinGlass");
  ospSetFloat(glassMaterial, "attenuationDistance", 0.2f);
  ospCommit(glassMaterial);

  ospSetObject(g_spheresModel, "material", glassMaterial);

  // commit the spheres geometry
  ospCommit(g_spheresGeometry);
  ospCommit(g_spheresModel);

  // release handles we no longer need
  ospRelease(radiusData);
  ospRelease(colorData);
  ospRelease(glassMaterial);

  return g_spheresModel;
}

void createWorld()
{
  // create the world which will contain all of our geometries
  g_world        = ospNewWorld();
  g_spheresGroup = ospNewGroup();

  std::vector<OSPInstance> instanceHandles;

  // add in spheres geometry (100 of them)
  g_spheresModel = createRandomSpheresGeometry(100);

  OSPData spheresModels = ospNewData(1, OSP_GEOMETRIC_MODEL, &g_spheresModel);
  ospSetObject(g_spheresGroup, "geometry", spheresModels);
  ospCommit(g_spheresGroup);

  OSPInstance spheresInstance = ospNewInstance(g_spheresGroup);
  ospCommit(spheresInstance);

  instanceHandles.push_back(spheresInstance);

  // add in a ground plane geometry
  OSPInstance plane = createGroundPlane(renderer_type);
  instanceHandles.push_back(plane);

  OSPData instances =
      ospNewData(instanceHandles.size(), OSP_INSTANCE, instanceHandles.data());
  ospSetObject(g_world, "instance", instances);
  ospRelease(instances);
  ospRelease(spheresInstance);
  ospRelease(plane);

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetObject(g_world, "light", lightsData);
  ospRelease(lightsData);

  ospCommit(g_world);
}

// updates the bouncing spheres coordinates based on simple gravity model
void updateSpheresCoordinates()
{
  // update the current spheres' heights based on current "time"
  static float t = 0.f;

  for (auto &s : g_spheres) {
    const float g    = 1.62f;  // moon surface gravity
    const float T    = sqrtf(8.f * s.maxHeight / g);
    const float Vmax = sqrtf(2.f * s.maxHeight * g);

    float tRemainder = ospcommon::math::fmod(0.5f * T + t, T);

    s.center.y = -1.f + s.radius - 0.5f * g * tRemainder * tRemainder +
                 Vmax * tRemainder;
  }

  // increment based on actual elapsed time
  static auto lastTime = std::chrono::high_resolution_clock::now();
  auto nowTime         = std::chrono::high_resolution_clock::now();

  float elaspedTimeSeconds =
      float(std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                                  lastTime)
                .count()) /
      1e6f;

  t += elaspedTimeSeconds;

  lastTime = nowTime;
}

void updateSpheresGeometry()
{
  // communicate that center coordinates got changed
  ospCommit(g_positionData);
  ospCommit(g_spheresGeometry);
}

// updates the bouncing spheres' coordinates, geometry, and world
void displayCallback(GLFWOSPRayWindow *glfwOSPRayWindow)
{
  // update the spheres coordinates and geometry
  updateSpheresCoordinates();
  updateSpheresGeometry();

  // queue the world to be committed since it changed, however don't commit
  // it immediately because it's being rendered asynchronously
  glfwOSPRayWindow->addObjectToCommit(g_spheresGeometry);
  glfwOSPRayWindow->addObjectToCommit(g_spheresGroup);
  glfwOSPRayWindow->addObjectToCommit(g_world);

  // update the world on the GLFW window
  glfwOSPRayWindow->setWorld(g_world);
}

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // create OSPRay world
  createWorld();

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), g_world, renderer));

  // register a callback with the GLFW OSPRay window to update the world every
  // frame
  glfwOSPRayWindow->registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)>(displayCallback));

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  ospRelease(g_positionData);
  ospRelease(g_spheresGeometry);
  ospRelease(g_spheresModel);
  ospRelease(g_spheresGroup);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
