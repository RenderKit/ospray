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

#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

using namespace ospcommon;

// creates a volume in the bounding box [(-1,-1,-1), (1,1,1)] with values based
// on weighted inverse-square distances to randomly generated points
OSPVolume createRandomVolume(size_t numPoints, size_t volumeDimension)
{
  struct Point
  {
    vec3f center;
    float weight;
  };

  // create random number distributions for point center and weight
  std::random_device rd;
  std::mt19937 gen(rd());

  std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
  std::uniform_real_distribution<float> weightDistribution(0.1f, 0.3f);

  // populate the points
  std::vector<Point> points(numPoints);

  for (auto &p : points) {
    p.center.x = centerDistribution(gen);
    p.center.y = centerDistribution(gen);
    p.center.z = centerDistribution(gen);

    p.weight = weightDistribution(gen);
  }

  // create a structured volume and assign attributes
  OSPVolume volume = ospNewVolume("block_bricked_volume");

  ospSet3i(
      volume, "dimensions", volumeDimension, volumeDimension, volumeDimension);
  ospSetString(volume, "voxelType", "float");
  ospSet3f(volume, "gridOrigin", -1.f, -1.f, -1.f);

  const float gridSpacing = 2.f / float(volumeDimension);
  ospSet3f(volume, "gridSpacing", gridSpacing, gridSpacing, gridSpacing);

  // get world coordinate in [-1.f, 1.f] from logical coordinates in [0,
  // volumeDimension)
  auto logicalToWorldCoordinates = [volumeDimension](
                                       size_t i, size_t j, size_t k) {
    return vec3f(-1.f + float(i) / float(volumeDimension - 1) * 2.f,
                 -1.f + float(j) / float(volumeDimension - 1) * 2.f,
                 -1.f + float(k) / float(volumeDimension - 1) * 2.f);
  };

  // generate volume values
  float *volumeData =
      new float[volumeDimension * volumeDimension * volumeDimension];

  for (size_t k = 0; k < volumeDimension; k++) {
    for (size_t j = 0; j < volumeDimension; j++) {
      for (size_t i = 0; i < volumeDimension; i++) {
        // index in array
        size_t index =
            k * volumeDimension * volumeDimension + j * volumeDimension + i;

        // compute volume value
        float value = 0.f;

        for (auto &p : points) {
          vec3f pointCoordinate = logicalToWorldCoordinates(i, j, k);
          const float distance  = length(pointCoordinate - p.center);

          // contribution proportional to weighted inverse-square distance (i.e.
          // gravity)
          value += p.weight / (distance * distance);
        }

        volumeData[index] = value;
      }
    }
  }

  // set the volume data
  ospSetRegion(
      volume,
      volumeData,
      osp::vec3i{0, 0, 0},
      osp::vec3i{
          int(volumeDimension), int(volumeDimension), int(volumeDimension)});

  // create a transfer function mapping volume values to color and opacity
  OSPTransferFunction transferFunction =
      ospNewTransferFunction("piecewise_linear");

  std::vector<vec3f> transferFunctionColors;
  std::vector<float> transferFunctionOpacities;

  transferFunctionColors.push_back(vec3f(0.f, 0.f, 1.f));
  transferFunctionColors.push_back(vec3f(0.f, 1.f, 0.f));
  transferFunctionColors.push_back(vec3f(1.f, 0.f, 0.f));

  transferFunctionOpacities.push_back(0.f);
  transferFunctionOpacities.push_back(0.01f);
  transferFunctionOpacities.push_back(0.05f);

  OSPData transferFunctionColorsData = ospNewData(
      transferFunctionColors.size(), OSP_FLOAT3, transferFunctionColors.data());
  OSPData transferFunctionOpacitiesData =
      ospNewData(transferFunctionOpacities.size(),
                 OSP_FLOAT,
                 transferFunctionOpacities.data());

  ospSetData(transferFunction, "colors", transferFunctionColorsData);
  ospSetData(transferFunction, "opacities", transferFunctionOpacitiesData);

  // the transfer function will apply over this volume value range
  ospSet2f(transferFunction, "valueRange", 0.f, 10.f);

  // commit the transfer function
  ospCommit(transferFunction);

  // set the transfer function on the volume
  ospSetObject(volume, "transferFunction", transferFunction);

  // enable gradient shading
  ospSet1i(volume, "gradientShadingEnabled", 1);

  // commit the volume
  ospCommit(volume);

  // release handles we no longer need
  ospRelease(transferFunction);
  ospRelease(transferFunctionColorsData);
  ospRelease(transferFunctionOpacitiesData);

  delete[] volumeData;

  return volume;
}

OSPModel createModel()
{
  // create the "world" model which will contain all of our geometries / volumes
  OSPModel world = ospNewModel();

  // add in randomly generated volume
  ospAddVolume(world, createRandomVolume(10, 256));

  // commit the world model
  ospCommit(world);

  return world;
}

OSPRenderer createRenderer()
{
  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("scivis");

  // create an ambient light
  OSPLight ambientLight = ospNewLight3("ambient");
  ospCommit(ambientLight);

  // create lights data containing all lights
  OSPLight lights[]  = {ambientLight};
  OSPData lightsData = ospNewData(1, OSP_LIGHT, lights, 0);
  ospCommit(lightsData);

  // complete setup of renderer
  ospSetData(renderer, "lights", lightsData);
  ospCommit(renderer);

  // release handles we no longer need
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

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
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

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
