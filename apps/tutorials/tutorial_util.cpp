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

#include "tutorial_util.h"

#include "ospcommon/math/vec.h"
using namespace ospcommon::math;

#include <iterator>
#include <vector>

void initializeOSPRay(int argc, const char **argv, bool errorsFatal)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    throw std::runtime_error("OSPRay not initialized correctly!");

  OSPDevice device = ospGetCurrentDevice();
  if (!device)
    throw std::runtime_error("OSPRay device could not be fetched!");

  // set an error callback to catch any OSPRay errors and exit the application
  if (errorsFatal) {
    ospDeviceSetErrorFunc(device, [](OSPError error, const char *errorDetails) {
      std::cerr << "OSPRay error: " << errorDetails << std::endl;
      exit(error);
    });
  } else {
    ospDeviceSetErrorFunc(device, [](OSPError, const char *errorDetails) {
      std::cerr << "OSPRay error: " << errorDetails << std::endl;
    });
  }
}

OSPInstance createGroundPlane(std::string renderer_type, float planeExtent)
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

  static std::vector<Vertex> vertices;
  static std::vector<QuadIndex> quadIndices;

  if (quadIndices.empty()) { // generate data on first use

    // ground plane
    int startingIndex = vertices.size();

    const vec3f up = vec3f{0.f, 1.f, 0.f};
    const vec4f gray = vec4f{0.9f, 0.9f, 0.9f, 0.75f};

    vertices.push_back(
        Vertex{vec3f{-planeExtent, -1.f, -planeExtent}, up, gray});
    vertices.push_back(
        Vertex{vec3f{planeExtent, -1.f, -planeExtent}, up, gray});
    vertices.push_back(Vertex{vec3f{planeExtent, -1.f, planeExtent}, up, gray});
    vertices.push_back(
        Vertex{vec3f{-planeExtent, -1.f, planeExtent}, up, gray});

    quadIndices.push_back(QuadIndex{startingIndex,
        startingIndex + 1,
        startingIndex + 2,
        startingIndex + 3});

    // stripes on ground plane
    const float stripeWidth = 0.025f;
    const float paddedExtent = planeExtent + stripeWidth;
    const size_t numStripes = 10;

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
  }

  // create OSPRay data objects
  OSPData positionData =
      ospNewSharedData((char *)vertices.data() + offsetof(Vertex, position),
          OSP_VEC3F,
          vertices.size(),
          sizeof(Vertex));
  OSPData normalData =
      ospNewSharedData((char *)vertices.data() + offsetof(Vertex, normal),
          OSP_VEC3F,
          vertices.size(),
          sizeof(Vertex));
  OSPData colorData =
      ospNewSharedData((char *)vertices.data() + offsetof(Vertex, color),
          OSP_VEC4F,
          vertices.size(),
          sizeof(Vertex));

  OSPData indexData =
      ospNewSharedData(quadIndices.data(), OSP_VEC4UI, quadIndices.size());

  // set vertex / index data on the geometry
  ospSetData(planeGeometry, "vertex.position", positionData);
  ospSetData(planeGeometry, "vertex.normal", normalData);
  ospSetData(planeGeometry, "vertex.color", colorData);
  ospSetData(planeGeometry, "index", indexData);

  // finally, commit the geometry
  ospCommit(planeGeometry);

  OSPGeometricModel model = ospNewGeometricModel(planeGeometry);

  ospRelease(planeGeometry);

  // create and assign a material to the geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospCommit(material);

  ospSetObject(model, "material", material);

  ospCommit(model);

  OSPGroup group = ospNewGroup();

  OSPData models = ospNewData(1, OSP_GEOMETRIC_MODEL, &model);
  ospSetData(group, "geometry", models);
  ospCommit(group);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // release handles we no longer need
  ospRelease(positionData);
  ospRelease(normalData);
  ospRelease(colorData);
  ospRelease(indexData);
  ospRelease(material);
  ospRelease(model);
  ospRelease(models);

  return instance;
}
