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

#include "ospcommon/library.h"
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
static OSPGeometry g_spheresGeometry;
static OSPGeometricModel g_spheresModel;
static OSPInstance g_instance;
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

  // create a data object with all the sphere information
  OSPData spheresData =
      ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, g_spheres.data());

  // create the sphere geometry, and assign attributes
  g_spheresGeometry = ospNewGeometry("spheres");
  g_spheresModel    = ospNewGeometricModel(g_spheresGeometry);

  ospSetData(g_spheresGeometry, "spheres", spheresData);
  ospSetInt(g_spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
  ospSetInt(g_spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
  ospSetInt(g_spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

  OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, g_colors.data());

  ospSetData(g_spheresModel, "color", colorData);

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
  ospRelease(spheresData);
  ospRelease(colorData);
  ospRelease(glassMaterial);

  return g_spheresModel;
}

void createWorld()
{
  // create the world which will contain all of our geometries
  g_world    = ospNewWorld();
  g_instance = ospNewInstance();

  std::vector<OSPInstance> instanceHandles;

  // add in spheres geometry (100 of them)
  g_spheresModel = createRandomSpheresGeometry(100);

  OSPData spheresModels = ospNewData(1, OSP_OBJECT, &g_spheresModel);
  ospSetData(g_instance, "geometries", spheresModels);
  ospCommit(g_instance);

  instanceHandles.push_back(g_instance);

  // add in a ground plane geometry
  OSPInstance plane = createGroundPlane(renderer_type);
  instanceHandles.push_back(plane);

  OSPData instances =
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());
  ospSetData(g_world, "instances", instances);
  ospRelease(instances);
  ospRelease(plane);

  ospCommit(g_world);
}

OSPRenderer createRenderer()
{
  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  // commit the renderer
  ospCommit(renderer);

  return renderer;
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

    float tRemainder = ospcommon::fmod(0.5f * T + t, T);

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
  // create new spheres data for the updated center coordinates, and assign to
  // geometry
  OSPData spheresData = ospNewData(
      g_spheres.size() * sizeof(Sphere), OSP_UCHAR, g_spheres.data());

  ospSetData(g_spheresGeometry, "spheres", spheresData);

  // release handles we no longer need
  ospRelease(spheresData);
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
  glfwOSPRayWindow->addObjectToCommit(g_spheresModel);
  glfwOSPRayWindow->addObjectToCommit(g_instance);
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
  OSPRenderer renderer = createRenderer();

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

  ospRelease(g_spheresGeometry);
  ospRelease(g_spheresModel);
  ospRelease(g_instance);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
