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
static OSPGeometryInstance g_spheresInstance;
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

OSPGeometryInstance createGroundPlane()
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

  // extent of plane in the (x, z) directions
  const float planeExtent = 1.5f;

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
      ospNewData(vertices.size(), OSP_FLOAT3, positionVector.data());
  OSPData normalData =
      ospNewData(vertices.size(), OSP_FLOAT3, normalVector.data());
  OSPData colorData =
      ospNewData(vertices.size(), OSP_FLOAT4, colorVector.data());
  OSPData indexData =
      ospNewData(quadIndices.size(), OSP_INT4, quadIndices.data());

  // set vertex / index data on the geometry
  ospSetData(planeGeometry, "vertex", positionData);
  ospSetData(planeGeometry, "vertex.normal", normalData);
  ospSetData(planeGeometry, "vertex.color", colorData);
  ospSetData(planeGeometry, "index", indexData);

  // finally, commit the geometry
  ospCommit(planeGeometry);

  OSPGeometryInstance planeInstance = ospNewGeometryInstance(planeGeometry);

  ospRelease(planeGeometry);

  // create and assign a material to the geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospCommit(material);

  ospSetMaterial(planeInstance, material);

  // release handles we no longer need
  ospRelease(positionData);
  ospRelease(normalData);
  ospRelease(colorData);
  ospRelease(indexData);
  ospRelease(material);

  ospCommit(planeInstance);

  return planeInstance;
}

OSPGeometryInstance createRandomSpheresGeometry(size_t numSpheres)
{
  g_spheres = generateRandomSpheres(numSpheres);

  // create a data object with all the sphere information
  OSPData spheresData =
      ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, g_spheres.data());

  // create the sphere geometry, and assign attributes
  g_spheresGeometry = ospNewGeometry("spheres");
  g_spheresInstance = ospNewGeometryInstance(g_spheresGeometry);

  ospSetData(g_spheresGeometry, "spheres", spheresData);
  ospSet1i(g_spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
  ospSet1i(g_spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
  ospSet1i(g_spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

  OSPData colorData = ospNewData(numSpheres, OSP_FLOAT4, g_colors.data());

  ospSetData(g_spheresInstance, "color", colorData);

  // create glass material and assign to geometry
  OSPMaterial glassMaterial =
      ospNewMaterial(renderer_type.c_str(), "ThinGlass");
  ospSet1f(glassMaterial, "attenuationDistance", 0.2f);
  ospCommit(glassMaterial);

  ospSetMaterial(g_spheresInstance, glassMaterial);

  // commit the spheres geometry
  ospCommit(g_spheresGeometry);
  ospCommit(g_spheresInstance);

  // release handles we no longer need
  ospRelease(spheresData);
  ospRelease(colorData);
  ospRelease(glassMaterial);

  return g_spheresInstance;
}

OSPWorld createWorld()
{
  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  std::vector<OSPGeometryInstance> instanceHandles;

  // add in spheres geometry (100 of them)
  OSPGeometryInstance instance = createRandomSpheresGeometry(100);
  instanceHandles.push_back(instance);

  // add in a ground plane geometry
  OSPGeometryInstance plane = createGroundPlane();
  instanceHandles.push_back(plane);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());

  ospSetData(world, "geometries", geomInstances);
  ospRelease(geomInstances);
  ospRelease(plane);

  // commit the world
  ospCommit(world);

  return world;
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
  glfwOSPRayWindow->addObjectToCommit(g_spheresInstance);
  glfwOSPRayWindow->addObjectToCommit(g_world);

  // update the world on the GLFW window
  glfwOSPRayWindow->setWorld(g_world);
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create OSPRay world
  g_world = createWorld();

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
  ospRelease(g_spheresInstance);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
