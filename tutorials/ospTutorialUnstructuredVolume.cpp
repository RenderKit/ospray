// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include <vector>
#include "GLFWOSPRayWindow.h"

#include <imgui.h>

using namespace ospcommon;

// Create a single unstructured volume consisting of three differently shaped
// cells: tetrahedron, hexahedron and wedge. 
OSPVolume createVolume()
{
  // create an unstructured volume object
  OSPVolume volume = ospNewVolume("unstructured_volume");

  // build and set volume cells
  {
    // define hexahedron parameters
    const float hSize = .5f;
    const float hX = -.5f, hY = -.5f, hZ = 0.f;

    // define wedge parameters
    const float wSize = .5f;
    const float wX = .5f, wY = -.5f, wZ = 0.f;

    // define tetrahedron parameters
    const float tSize = .5f;
    const float tX = .5f, tY = .5f, tZ = 0.f;

    // define vertex positions, duplicated vertices
    // that can be shared between cells are commented out
    std::vector<vec3f> vertices =
    {
        // hexahedron bottom quad
        { -hSize + hX, -hSize + hY,  hSize + hZ },    // 0
        {  hSize + hX, -hSize + hY,  hSize + hZ },    // 1
        {  hSize + hX, -hSize + hY, -hSize + hZ },    // 2
        { -hSize + hX, -hSize + hY, -hSize + hZ },    // 3
        // hexahedron top quad
        { -hSize + hX,  hSize + hY,  hSize + hZ },    // 4
        {  hSize + hX,  hSize + hY,  hSize + hZ },    // 5
        {  hSize + hX,  hSize + hY, -hSize + hZ },    // 6
        { -hSize + hX,  hSize + hY, -hSize + hZ },    // 7

        // wedge bottom triangle, sharing 2 hexahedron vertices
        // { -wSize + wX, -wSize + wY, -wSize + wZ }, // 1
        {  wSize + wX, -wSize + wY,    0.f + wZ },    // 8
        // { -wSize + wX, -wSize + wY,  wSize + wZ }, // 2
        // wedge top triangle, sharing 2 hexahedron vertices
        // { -wSize + wX,  wSize + wY, -wSize + wZ }, // 5
        {  wSize + wX,  wSize + wY,    0.f + wZ },    // 9
        // { -wSize + wX,  wSize + wY,  wSize + wZ }, // 6

        // tetrahedron, sharing 3 vertices
        // { -tSize + tX, -tSize + tY, -tSize + tZ }, // 5
        // {  tSize + tX, -tSize + tY,    0.f + tZ }, // 9
        // { -tSize + tX, -tSize + tY,  tSize + tZ }, // 6
        {    0.f + tX,  tSize + tY,    0.f + tZ }     // 10
    };

    // define cell field value
    std::vector<float> cellFields = { 0.2f, 5.f, 9.8f };

    // shape cells by defining indices
    std::vector<vec4i> indices =
    {
        // hexahedron
        {  0,  1,  2,  3 },
        {  4,  5,  6,  7 },

        // wedge
        { -2, -2,  1,  8 },
        {  2,  5,  9,  6 },

        // tetrahedron
        { -1, -1, -1, -1 },
        {  5,  9,  6, 10 }
    };

    // create data objects
    OSPData verticesData = ospNewData(
        vertices.size(), OSP_FLOAT3, vertices.data());
    OSPData cellFieldsData = ospNewData(
        cellFields.size(), OSP_FLOAT, cellFields.data());
    OSPData indicesData = ospNewData(
        indices.size(), OSP_INT4, indices.data());

    // set data objects for volume object
    ospSetData(volume, "vertices", verticesData);
    ospSetData(volume, "cellField", cellFieldsData);
    ospSetData(volume, "indices", indicesData);

    // release handlers that go out of scope here
    ospRelease(verticesData);
    ospRelease(cellFieldsData);
    ospRelease(indicesData);
  }

  // build and apply transfer function
  {
    // create transfer function object
    OSPTransferFunction transferFunction =
        ospNewTransferFunction("piecewise_linear");

    // prepare color and opacity data
    std::vector<vec3f> tfColors =
    {
        { 0.f, 0.f, 1.f },
        { 0.f, 1.f, 0.f },
        { 1.f, 0.f, 0.f }
    };
    std::vector<float> tfOpacities = { 0.01f, 0.01f, 0.01f };

    // create data objects
    OSPData tfColorsData = ospNewData(tfColors.size(),
        OSP_FLOAT3, tfColors.data());
    OSPData tfOpacitiesData = ospNewData(tfOpacities.size(),
        OSP_FLOAT, tfOpacities.data());

    // set data objects for transfer function object
    ospSetData(transferFunction, "colors", tfColorsData);
    ospSetData(transferFunction, "opacities", tfOpacitiesData);

    ospSet2f(transferFunction, "valueRange", 0.f, 10.f);

    // apply changes to the transfer function
    ospCommit(transferFunction);

    // set the transfer function on the volume
    ospSetObject(volume, "transferFunction", transferFunction);

    // release handlers that go out of scope here
    ospRelease(tfColorsData);
    ospRelease(tfOpacitiesData);
    ospRelease(transferFunction);
  }

  // apply changes to the volume
  ospCommit(volume);

  // done
  return volume;
}

OSPModel createModel()
{
  // create the "world" model which will contain all of our geometries / volumes
  OSPModel world = ospNewModel();

  // generate a volume and add to the world model
  OSPVolume volume = createVolume();
  ospAddVolume(world, volume);
  ospRelease(volume);

  // commit the world model
  ospCommit(world);

  // done
  return world;
}

OSPRenderer createRenderer()
{
  // Create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("scivis");

  // Create an ambient light
  OSPLight ambientLight = ospNewLight3("ambient");
  ospCommit(ambientLight);

  // Create lights data containing all lights
  OSPLight lights[]  = {ambientLight};
  OSPData lightsData = ospNewData(1, OSP_LIGHT, lights, 0);
  ospCommit(lightsData);

  // Complete setup of renderer
  ospSetData(renderer, "lights", lightsData);
  ospCommit(renderer);

  // Release handles we no longer need
  ospRelease(lightsData);

  
  return renderer;
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // get OSPRay device
  OSPDevice ospDevice = ospGetCurrentDevice();
  if (!ospDevice)
    return -1;

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospDevice, [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create OSPRay model
  OSPModel model = createModel();

  // create OSPRay renderer
  OSPRenderer renderer = createRenderer();

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), model, renderer));

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
