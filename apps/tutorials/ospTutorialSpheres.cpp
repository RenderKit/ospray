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

#include <imgui.h>

using namespace ospcommon;

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

  // create the "world" model which will contain all of our geometries
  OSPModel world = ospNewModel();

  // add in spheres geometry
  OSPTestingGeometry spheres = ospTestingNewGeometry("spheres", "pathtracer");
  ospAddGeometry(world, spheres.geometry);
  ospRelease(spheres.geometry);

  // add in a ground plane geometry
  ospAddGeometry(world, createGroundPlaneGeometry());

  // commit the world model
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("pathtracer");

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([=]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      ospCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
