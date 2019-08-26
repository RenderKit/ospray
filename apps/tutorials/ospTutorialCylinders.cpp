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

static std::string renderer_type = "scivis";

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();
  std::vector<OSPInstance> instanceHandles;

  // Create Cylinder struct
  struct Cylinder
  {
    vec3f startVertex;
    vec3f endVertex;
    float radius;
  };

  // create random number distributions for cylinder start, end, radius, color
  std::random_device rd;
  std::mt19937 gen(rd());

  std::uniform_real_distribution<float> startDelta(-0.015625f, 0.015625f);
  std::uniform_real_distribution<float> endDelta(-0.125f, 0.125f);
  std::uniform_real_distribution<float> radiusDistribution(0.001f, 0.002f);
  std::uniform_real_distribution<float> colorDelta(-0.1f, 0.1f);

  // Set up our patches of grass
  vec2i numPatches = vec2i{9, 9};
  int numCylindersPerPatch = 32;
  int numCylinders = numPatches.x * numPatches.y * numCylindersPerPatch;

  // populate the cylinders
  std::vector<Cylinder> cylinders(numCylinders);
  std::vector<vec4f> colors(numCylinders);

  for (int pz = 0; pz < numPatches.y; pz++) {
    for (int px = 0; px < numPatches.x; px++) {
      for (int ps = 0; ps < numCylindersPerPatch; ps++) {
        Cylinder& s 
            = cylinders.at((pz*numPatches.x + px)*numCylindersPerPatch + ps);
        s.startVertex.x = 
            (4.0f/numPatches.x)*(px+0.5f) - 2.0f + startDelta(gen);
        s.startVertex.y = -1.0f;
        s.startVertex.z = 
            (4.0f/numPatches.y)*(pz+0.5f) - 2.0f + startDelta(gen);

        s.endVertex.x = 
            (4.0f/numPatches.x)*(px+0.5f) - 2.0f + endDelta(gen);
        s.endVertex.y = -0.5f + endDelta(gen);
        s.endVertex.z = 
            (4.0f/numPatches.y)*(pz+0.5f) - 2.0f + endDelta(gen);

        s.radius = radiusDistribution(gen);
      } 
    }
  }
  
  // Make all cylinders a greenish hue, with some luma variance
  for (auto &c : colors) {
    c.x = 0.2f + colorDelta(gen);
    c.y = 0.6f + colorDelta(gen);
    c.z = 0.1f + colorDelta(gen);
    c.w = 1.0f;
  }

  // create a data object with all the cylinder information
  OSPData cylindersData = 
      ospNewData(numCylinders * sizeof(Cylinder), OSP_UCHAR, cylinders.data());

  // create the cylinder geometry, and assign attributes
  OSPGeometry cylindersGeometry = ospNewGeometry("cylinders");

  ospSetData(cylindersGeometry, "cylinder", cylindersData);
  ospSetInt(cylindersGeometry, "bytes_per_cylinder", int(sizeof(Cylinder)));
  ospSetInt(
      cylindersGeometry, "offset_v0", int(offsetof(Cylinder, startVertex)));
  ospSetInt(
      cylindersGeometry, "offset_v1", int(offsetof(Cylinder, endVertex)));
  ospSetInt(
      cylindersGeometry, "offset_radius", int(offsetof(Cylinder, radius)));

  // commit the cylinders geometry
  ospCommit(cylindersGeometry);

  // Add the color data to the cylinders
  OSPGeometricModel cylindersModel = ospNewGeometricModel(cylindersGeometry);
  OSPData cylindersColor = ospNewData(numCylinders, OSP_VEC4F, colors.data());
  ospSetData(cylindersModel, "prim.color", cylindersColor);

  // create obj material and assign to cylinders model
  OSPMaterial objMaterial =
    ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  //ospSetVec3f(objMaterial, "Ks", 0.8f, 0.8f, 0.8f);
  ospCommit(objMaterial);

  ospSetObject(cylindersModel, "material", objMaterial);

  // release handles we no longer need
  ospRelease(cylindersData);
  ospRelease(cylindersColor);
  ospRelease(objMaterial);

  ospCommit(cylindersModel);

  OSPGroup group = ospNewGroup();
  auto models    = ospNewData(1, OSP_OBJECT, &cylindersModel);
  ospSetData(group, "geometry", models);
  ospCommit(group);
  ospRelease(models);

  // Codify cylinders into an instance
  OSPInstance cylindersInstance = ospNewInstance(group);
  ospCommit(cylindersInstance);

  instanceHandles.push_back(cylindersInstance);
  ospRelease(cylindersGeometry);
  ospRelease(cylindersModel);

  // add in a ground plane geometry
  OSPInstance planeInstance = createGroundPlane(renderer_type, 2.0f);
  instanceHandles.push_back(planeInstance);

  // Add the plane and cylinders instances to the world
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
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
