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

using namespace ospcommon;

static std::string renderer_type = "pathtracer";

// quad mesh data
static float vertices[] = { 
  // Floor
   1.00f, -1.00f, -1.00f,
  -1.00f, -1.00f, -1.00f,
  -1.00f, -1.00f,  1.00f,
   1.00f, -1.00f,  1.00f,
  // Ceiling
   1.00f,  1.00f, -1.00f,
   1.00f,  1.00f,  1.00f,
  -1.00f,  1.00f,  1.00f,
  -1.00f,  1.00f, -1.00f,
  // Backwall
   1.00f, -1.00f,  1.00f,
  -1.00f, -1.00f,  1.00f,
  -1.00f,  1.00f,  1.00f,
   1.00f,  1.00f,  1.00f,
  // RightWall
  -1.00f, -1.00f,  1.00f,
  -1.00f, -1.00f, -1.00f,
  -1.00f,  1.00f, -1.00f,
  -1.00f,  1.00f,  1.00f,
  // LeftWall
   1.00f, -1.00f, -1.00f,
   1.00f, -1.00f,  1.00f,
   1.00f,  1.00f,  1.00f,
   1.00f,  1.00f, -1.00f,
  // ShortBox Top Face
  -0.53f, -0.40f, -0.75f,
  -0.70f, -0.40f, -0.17f,
  -0.13f, -0.40f, -0.00f,
   0.05f, -0.40f, -0.57f,
  // ShortBox Left Face
   0.05f, -1.00f, -0.57f,
   0.05f, -0.40f, -0.57f,
  -0.13f, -0.40f, -0.00f,
  -0.13f, -1.00f, -0.00f,
  // ShortBox Front Face
  -0.53f, -1.00f, -0.75f,
  -0.53f, -0.40f, -0.75f,
   0.05f, -0.40f, -0.57f,
   0.05f, -1.00f, -0.57f,
  // ShortBox Right Face
  -0.70f, -1.00f, -0.17f,
  -0.70f, -0.40f, -0.17f,
  -0.53f, -0.40f, -0.75f,
  -0.53f, -1.00f, -0.75f,
  // ShortBox Back Face
  -0.13f, -1.00f, -0.00f,
  -0.13f, -0.40f, -0.00f,
  -0.70f, -0.40f, -0.17f,
  -0.70f, -1.00f, -0.17f,
  // ShortBox Bottom Face
  -0.53f, -1.00f, -0.75f,
  -0.70f, -1.00f, -0.17f,
  -0.13f, -1.00f, -0.00f,
   0.05f, -1.00f, -0.57f,
  // TallBox Top Face
   0.53f,  0.20f, -0.09f,
  -0.04f,  0.20f,  0.09f,
   0.14f,  0.20f,  0.67f,
   0.71f,  0.20f,  0.49f,
  // TallBox Left Face
   0.53f, -1.00f, -0.09f,
   0.53f,  0.20f, -0.09f,
   0.71f,  0.20f,  0.49f,
   0.71f, -1.00f,  0.49f,
  // TallBox Front Face
   0.71f, -1.00f,  0.49f,
   0.71f,  0.20f,  0.49f,
   0.14f,  0.20f,  0.67f,
   0.14f, -1.00f,  0.67f,
  // TallBox Right Face
   0.14f, -1.00f,  0.67f,
   0.14f,  0.20f,  0.67f,
  -0.04f,  0.20f,  0.09f,
  -0.04f, -1.00f,  0.09f,
  // TallBox Back Face
  -0.04f, -1.00f,  0.09f,
  -0.04f,  0.20f,  0.09f,
   0.53f,  0.20f, -0.09f,
   0.53f, -1.00f, -0.09f,
  // TallBox Bottom Face
   0.53f, -1.00f, -0.09f,
  -0.04f, -1.00f,  0.09f,
   0.14f, -1.00f,  0.67f,
   0.71f, -1.00f,  0.49f
};  

static int32_t indices[] = {
  // Floor
   0,  1,  2,  3,
  // Ceiling
   4,  5,  6,  7,
  // Backwall
   8,  9, 10, 11,
  // RightWall
  12, 13, 14, 15,
  // LeftWall
  16, 17, 18, 19,
  // ShortBox Top Face
  20, 21, 22, 23,
  // ShortBox Left Face
  24, 25, 26, 27,
  // ShortBox Front Face
  28, 29, 30, 31,
  // ShortBox Right Face
  32, 33, 34, 35,
  // ShortBox Back Face
  36, 37, 38, 39,
  // ShortBox Bottom Face
  40, 41, 42, 43,
  // TallBox Top Face
  44, 45, 46, 47,
  // TallBox Left Face
  48, 49, 50, 51,
  // TallBox Front Face
  52, 53, 54, 55,
  // TallBox Right Face
  56, 57, 58, 59,
  // TallBox Back Face
  60, 61, 62, 63,
  // TallBox Bottom Face
  64, 65, 66, 67
};

static float colors[] = {
  // Floor
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // Ceiling
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // Backwall
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // RightWall
  0.140f, 0.450f, 0.091f, 1.0f,
  0.140f, 0.450f, 0.091f, 1.0f,
  0.140f, 0.450f, 0.091f, 1.0f,
  0.140f, 0.450f, 0.091f, 1.0f,
  // LeftWall
  0.630f, 0.065f, 0.05f, 1.0f,
  0.630f, 0.065f, 0.05f, 1.0f,
  0.630f, 0.065f, 0.05f, 1.0f,
  0.630f, 0.065f, 0.05f, 1.0f,
  // ShortBox Top Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // ShortBox Left Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // ShortBox Front Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // ShortBox Right Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // ShortBox Back Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // ShortBox Bottom Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Top Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Left Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Front Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Right Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Back Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  // TallBox Bottom Face
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f,
  0.725f, 0.710f, 0.68f, 1.0f
};


int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // create and setup mesh
  OSPGeometry quadMesh = ospNewGeometry("quads");

  OSPData quadVerts   = ospNewData(17*4, OSP_VEC3F, vertices, 0);
  OSPData quadColors  = ospNewData(17*4, OSP_VEC4F, colors, 0);
  OSPData quadIndices = ospNewData(17, OSP_VEC4I, indices, 0);
  
  ospCommit(quadVerts);
  ospCommit(quadColors);
  ospCommit(quadIndices);

  ospSetData(quadMesh, "vertex.position", quadVerts);
  ospSetData(quadMesh, "vertex.color", quadColors);
  ospSetData(quadMesh, "index", quadIndices);

  ospCommit(quadMesh);
  ospRelease(quadVerts);
  ospRelease(quadColors);
  ospRelease(quadIndices);

  // create and setup a material
  OSPMaterial quadMeshMaterial = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospCommit(quadMeshMaterial);
  
  // Put the mesh and material into a model
  OSPGeometricModel quadMeshModel = ospNewGeometricModel(quadMesh);
  ospSetObject(quadMeshModel,"material", quadMeshMaterial);
  ospCommit(quadMeshModel);
  ospRelease(quadMesh);
  ospRelease(quadMeshMaterial);

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  OSPData geometricModels = ospNewData(1, OSP_OBJECT, &quadMeshModel, 0);
  ospSetData(group, "geometry", geometricModels);
  
  ospCommit(group);
  ospRelease(quadMeshModel);
  ospRelease(geometricModels);

  // put the group into an instance (give the group a world transform)
  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  OSPWorld world = ospNewWorld();
  OSPData instances = ospNewData(1, OSP_OBJECT, &instance, 0);
  ospSetData(world, "instance", instances);
  ospCommit(world);
  ospRelease(instance);
  ospRelease(instances);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  // Set up area light in the ceiling
  OSPLight light = ospNewLight("quad");
  ospSetVec3f(light, "color", 0.78f, 0.551f, 0.183f);
  ospSetFloat(light, "intensity", 47.f);
  ospSetVec3f(light, "position", -0.23f, 0.98f, -0.16f); 
  ospSetVec3f(light, "edge1", 0.47f, 0.0f, 0.0f);
  ospSetVec3f(light, "edge2", 0.0f, 0.0f, 0.38f);

  ospCommit(light);
  OSPData lights = ospNewData(1, OSP_LIGHT, &light, 0);
  ospCommit(lights);
  ospSetData(renderer, "light", lights);
 
  // finalize the renderer
  ospCommit(renderer);
  ospRelease(light);
  ospRelease(lights);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 1024}, box3f(vec3f(-1.0f), vec3f(1.0f)), world, renderer));

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}





