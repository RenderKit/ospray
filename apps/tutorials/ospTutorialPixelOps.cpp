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

using namespace ospcommon;

static std::string renderer_type = "pathtracer";

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

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  std::vector<OSPGeometryInstance> instanceHandles;

  // add in spheres geometry
  OSPTestingGeometry spheres =
      ospTestingNewGeometry("spheres", renderer_type.c_str());
  instanceHandles.push_back(spheres.instance);
  ospRelease(spheres.geometry);

  // add in a ground plane geometry
  OSPGeometryInstance planeInstance = createGroundPlane();
  ospCommit(planeInstance);
  instanceHandles.push_back(planeInstance);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());

  ospSetData(world, "geometries", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // Create a PixelOp pipeline to apply to the image
  OSPPixelOp toneMap = ospNewPixelOp("tonemapper");
  ospCommit(toneMap);

  // The colortweak pixel op will make the image more blue
  OSPPixelOp colorTweak = ospNewPixelOp("debug");
  ospSet3f(colorTweak, "addColor", 0.0, 0.0, 0.2);
  ospCommit(colorTweak);

  // UI for the tweaking the pixel pipeline
  std::vector<bool> enabledOps   = {true, true};
  std::vector<vec3f> debugColors = {vec3f(-1.f), vec3f(0, 0, 0.2)};

  std::vector<OSPPixelOp> pixelPipeline = {toneMap, colorTweak};
  OSPData pixelOpData =
      ospNewData(pixelPipeline.size(), OSP_OBJECT, pixelPipeline.data());
  ospCommit(pixelOpData);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->setPixelOps(pixelOpData);

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool pixelOpsUpdated = false;
    for (size_t i = 0; i < pixelPipeline.size(); ++i) {
      ImGui::PushID(i);

      bool enabled = enabledOps[i];
      if (ImGui::Checkbox("Enabled", &enabled)) {
        enabledOps[i]   = enabled;
        pixelOpsUpdated = true;
      }

      if (debugColors[i] != vec3f(-1.f)) {
        ImGui::Text("Debug Pixel Op #%lu", i);
        if (ImGui::ColorPicker3("Color", &debugColors[i].x)) {
          ospSet3f(pixelPipeline[i],
                   "addColor",
                   debugColors[i].x,
                   debugColors[i].y,
                   debugColors[i].z);
          pixelOpsUpdated = true;
          glfwOSPRayWindow->addObjectToCommit(pixelPipeline[i]);
        }
      } else {
        ImGui::Text("Tonemapper Pixel Op #%lu", i);
      }

      ImGui::PopID();
    }

    if (ImGui::Button("Add Debug Op")) {
      OSPPixelOp op = ospNewPixelOp("debug");
      ospSet3f(op, "addColor", 0.0, 0.0, 0.0);
      ospCommit(op);

      pixelPipeline.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(0));

      pixelOpsUpdated = true;
    }
    if (ImGui::Button("Add Tonemap Op")) {
      OSPPixelOp op = ospNewPixelOp("tonemapper");
      ospCommit(op);

      pixelPipeline.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(-1));

      pixelOpsUpdated = true;
    }

    if (!pixelPipeline.empty() && ImGui::Button("Remove Op")) {
      ospRelease(pixelPipeline.back());
      pixelPipeline.pop_back();
      enabledOps.pop_back();
      debugColors.pop_back();

      pixelOpsUpdated = true;
    }

    if (pixelOpsUpdated) {
      ospRelease(pixelOpData);
      std::vector<OSPPixelOp> enabled;
      for (size_t i = 0; i < pixelPipeline.size(); ++i) {
        if (enabledOps[i])
          enabled.push_back(pixelPipeline[i]);
      }

      pixelOpData = ospNewData(enabled.size(), OSP_OBJECT, enabled.data());
      ospCommit(pixelOpData);
      glfwOSPRayWindow->setPixelOps(pixelOpData);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(pixelOpData);

  for (auto &op : pixelPipeline)
    ospRelease(op);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
