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
#include "ospcommon/FileName.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/range.h"
#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

static const std::string renderer_type = "scivis";

// ALOK: AMRVolume context info
std::vector<float *> brickPtrs;            // holds actual data
std::vector<OSPData> brickData;            // holds actual data as OSPData
range1f valueRange;
float *actualData;
box3f bounds;
int maxParseLevel;

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // we must load the testing library explicitly on Windows to look up
  // object creation functions
  loadLibrary("ospray_testing");

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  std::vector<OSPGeometryInstance> geometryInstances;

  // add in AMR volume
  OSPTestingVolume testData = ospTestingNewVolume("gravity_spheres_amr_volume");

  OSPVolume amrVolume = testData.volume;

  // create a volume instance container
  OSPVolumeInstance instance = ospNewVolumeInstance(amrVolume);
  ospSetFloat(instance, "samplingRate", 10.f);

  // apply a transfer function to the instance
  osp_vec2f amrValueRange = testData.voxelRange;
  OSPTransferFunction tf = ospTestingNewTransferFunction(amrValueRange, "jet");
  ospSetObject(instance, "transferFunction", tf);
  ospCommit(instance);

  // create a data array of all instances
  OSPData volumeInstances = ospNewData(1, OSP_OBJECT, &instance);

  // add volume instance to the world
  ospSetObject(world, "volumes", volumeInstances);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  bounds = reinterpret_cast<box3f &>(testData.bounds);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, bounds, world, renderer));

  // commit the world
  ospCommit(world);

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
