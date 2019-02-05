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

#include <imgui.h>

using namespace ospcommon;

struct Sphere
{
  // initial (maximum) height of the sphere, at which it has 0 velocity
  float maxHeight;

  vec3f center;
  float radius;
  vec4f color;
};

// track the spheres data, geometry, and model globally so we can update them
// every frame
static std::vector<Sphere> g_spheres;
static OSPGeometry g_spheresGeometry;
static OSPModel g_model;

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

  for (auto &s : spheres) {
    // maximum height above y=-1 ground plane
    s.maxHeight = 1.f + centerDistribution(gen);

    s.center.x = centerDistribution(gen);
    s.center.y = s.maxHeight;
    s.center.z = centerDistribution(gen);

    s.radius = radiusDistribution(gen);

    s.color.x = colorDistribution(gen);
    s.color.y = colorDistribution(gen);
    s.color.z = colorDistribution(gen);
    s.color.w = colorDistribution(gen);
  }

  return spheres;
}

OSPGeometry createGroundPlaneGeometry()
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

  // create and assign a material to the geometry
  OSPMaterial material = ospNewMaterial2("pathtracer", "OBJMaterial");
  ospCommit(material);

  ospSetMaterial(planeGeometry, material);

  // finally, commit the geometry
  ospCommit(planeGeometry);

  // release handles we no longer need
  ospRelease(positionData);
  ospRelease(normalData);
  ospRelease(colorData);
  ospRelease(indexData);
  ospRelease(material);

  return planeGeometry;
}

OSPGeometry createRandomSpheresGeometry(size_t numSpheres)
{
  g_spheres = generateRandomSpheres(numSpheres);

  // create a data object with all the sphere information
  OSPData spheresData =
      ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, g_spheres.data());

  // create the sphere geometry, and assign attributes
  g_spheresGeometry = ospNewGeometry("spheres");

  ospSetData(g_spheresGeometry, "spheres", spheresData);
  ospSet1i(g_spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
  ospSet1i(g_spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
  ospSet1i(g_spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

  ospSetData(g_spheresGeometry, "color", spheresData);
  ospSet1i(g_spheresGeometry, "color_offset", int(offsetof(Sphere, color)));
  ospSet1i(g_spheresGeometry, "color_format", int(OSP_FLOAT4));
  ospSet1i(g_spheresGeometry, "color_stride", int(sizeof(Sphere)));

  // create glass material and assign to geometry
  OSPMaterial glassMaterial = ospNewMaterial2("pathtracer", "ThinGlass");
  ospSet1f(glassMaterial, "attenuationDistance", 0.2f);
  ospCommit(glassMaterial);

  ospSetMaterial(g_spheresGeometry, glassMaterial);

  // commit the spheres geometry
  ospCommit(g_spheresGeometry);

  // release handles we no longer need
  ospRelease(spheresData);
  ospRelease(glassMaterial);

  return g_spheresGeometry;
}

OSPModel createModel()
{
  // create the "world" model which will contain all of our geometries
  OSPModel world = ospNewModel();

  // add in spheres geometry (100 of them)
  ospAddGeometry(world, createRandomSpheresGeometry(100));

  // add in a ground plane geometry
  ospAddGeometry(world, createGroundPlaneGeometry());

  // commit the world model
  ospCommit(world);

  return world;
}

OSPRenderer createRenderer()
{
  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("pathtracer");

  // create an ambient light
  OSPLight ambientLight = ospNewLight3("ambient");
  ospCommit(ambientLight);

  // create lights data containing all lights
  OSPLight lights[]  = {ambientLight};
  OSPData lightsData = ospNewData(1, OSP_LIGHT, lights, 0);
  ospCommit(lightsData);

  // set the lights to the renderer
  ospSetData(renderer, "lights", lightsData);

  // commit the renderer
  ospCommit(renderer);

  // release handles we no longer need
  ospRelease(lightsData);

  return renderer;
}

// updates the bouncing spheres coordinates based on simple gravity model
void updateSpheresCoordinates()
{
  // update the current spheres' heights based on current "time"
  static float t = 0.f;

  for (auto &s : g_spheres) {
    const float g    = 9.81f;
    const float T    = sqrtf(8.f * s.maxHeight / g);
    const float Vmax = sqrtf(2.f * s.maxHeight * g);

    float tRemainder = ospcommon::fmod(0.5f * T + t, T);

    s.center.y = -1.f + s.radius - 0.5f * g * tRemainder * tRemainder +
                 Vmax * tRemainder;
  }

  // increment by fixed time interval, rather than actual elapsed time
  t += 0.01f;
}

void updateSpheresGeometry()
{
  // create new spheres data for the updated center coordinates, and assign to
  // geometry
  OSPData spheresData = ospNewData(
      g_spheres.size() * sizeof(Sphere), OSP_UCHAR, g_spheres.data());

  ospSetData(g_spheresGeometry, "spheres", spheresData);

  // commit the updated spheres geometry
  ospCommit(g_spheresGeometry);

  // release handles we no longer need
  ospRelease(spheresData);
}

// updates the bouncing spheres' coordinates, geometry, and model
void displayCallback(GLFWOSPRayWindow *glfwOSPRayWindow)
{
  // update the spheres coordinates and geometry
  updateSpheresCoordinates();
  updateSpheresGeometry();

  // commit the model since the spheres geometry changed
  ospCommit(g_model);

  // update the model on the GLFW window
  glfwOSPRayWindow->setModel(g_model);
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create OSPRay model
  g_model = createModel();

  // create OSPRay renderer
  OSPRenderer renderer = createRenderer();

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), g_model, renderer));

  // register a callback with the GLFW OSPRay window to update the model every
  // frame
  glfwOSPRayWindow->registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)>(displayCallback));

  glfwOSPRayWindow->registerImGuiCallback([=]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      ospCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
