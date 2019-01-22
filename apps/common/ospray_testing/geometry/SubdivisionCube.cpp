// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Geometry.h"
// ospcommon
#include "ospcommon/box.h"
using namespace ospcommon;

#include <vector>

namespace ospray {
  namespace testing {

    struct SubdivisionCube : public Geometry
    {
      SubdivisionCube()           = default;
      ~SubdivisionCube() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingGeometry SubdivisionCube::createGeometry(
        const std::string &) const
    {
      box3f bounds(-1.f, 1.f);

      // vertices of a cube
      std::vector<vec3f> vertices = {{-1.0f, -1.0f, -1.0f},
                                     {1.0f, -1.0f, -1.0f},
                                     {1.0f, -1.0f, 1.0f},
                                     {-1.0f, -1.0f, 1.0f},
                                     {-1.0f, 1.0f, -1.0f},
                                     {1.0f, 1.0f, -1.0f},
                                     {1.0f, 1.0f, 1.0f},
                                     {-1.0f, 1.0f, 1.0f}};

      // color per vertex
      std::vector<vec4f> colors = {{0.0f, 0.0f, 0.0f, 1.f},
                                   {1.0f, 0.0f, 0.0f, 1.f},
                                   {1.0f, 0.0f, 1.0f, 1.f},
                                   {0.0f, 0.0f, 1.0f, 1.f},
                                   {0.0f, 1.0f, 0.0f, 1.f},
                                   {1.0f, 1.0f, 0.0f, 1.f},
                                   {1.0f, 1.0f, 1.0f, 1.f},
                                   {0.0f, 1.0f, 1.0f, 1.f}};

      // number of vertex indices per face
      std::vector<unsigned int> faces = {4, 4, 4, 4, 4, 4};

      // vertex indices for all faces
      std::vector<unsigned int> indices = {
          0, 4, 5, 1, 1, 5, 6, 2, 2, 6, 7, 3,
          0, 3, 7, 4, 4, 7, 6, 5, 0, 1, 2, 3,
      };

      // vertex crease indices and weights
      std::vector<unsigned int> vertexCreaseIndices = {0, 1, 2, 3, 4, 5, 6, 7};
      std::vector<float> vertexCreaseWeights        = {
          2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f};

      // edge crease indices and weights
      std::vector<vec2i> edgeCreaseIndices = {{0, 1},
                                              {1, 2},
                                              {2, 3},
                                              {3, 0},
                                              {4, 5},
                                              {5, 6},
                                              {6, 7},
                                              {7, 4},
                                              {0, 4},
                                              {1, 5},
                                              {2, 6},
                                              {3, 7}};
      std::vector<float> edgeCreaseWeights = {
          2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f, 2.f};

      // global level of tessellation
      float level = 5.f;

      // create the OSPRay geometry and set all parameters
      OSPGeometry geometry = ospNewGeometry("subdivision");

      OSPData verticesData =
          ospNewData(vertices.size(), OSP_FLOAT3, vertices.data());
      ospSetData(geometry, "vertex", verticesData);
      ospRelease(verticesData);

      OSPData colorsData = ospNewData(colors.size(), OSP_FLOAT4, colors.data());
      ospSetData(geometry, "color", colorsData);
      ospRelease(colorsData);

      OSPData facesData = ospNewData(faces.size(), OSP_UINT, faces.data());
      ospSetData(geometry, "face", facesData);
      ospRelease(facesData);

      OSPData indicesData =
          ospNewData(indices.size(), OSP_UINT, indices.data());
      ospSetData(geometry, "index", indicesData);
      ospRelease(indicesData);

      OSPData vertexCreaseIndicesData = ospNewData(
          vertexCreaseIndices.size(), OSP_UINT, vertexCreaseIndices.data());
      ospSetData(geometry, "vertexCrease.index", vertexCreaseIndicesData);
      ospRelease(vertexCreaseIndicesData);

      OSPData vertexCreaseWeightsData = ospNewData(
          vertexCreaseWeights.size(), OSP_FLOAT, vertexCreaseWeights.data());
      ospSetData(geometry, "vertexCrease.weight", vertexCreaseWeightsData);
      ospRelease(vertexCreaseWeightsData);

      OSPData edgeCreaseIndicesData = ospNewData(
          edgeCreaseIndices.size(), OSP_INT2, edgeCreaseIndices.data());
      ospSetData(geometry, "edgeCrease.index", edgeCreaseIndicesData);
      ospRelease(edgeCreaseIndicesData);

      OSPData edgeCreaseWeightsData = ospNewData(
          edgeCreaseWeights.size(), OSP_FLOAT, edgeCreaseWeights.data());
      ospSetData(geometry, "edgeCrease.weight", edgeCreaseWeightsData);
      ospRelease(edgeCreaseWeightsData);

      ospSet1f(geometry, "level", level);

      // create OBJ material and assign to geometry
      OSPMaterial objMaterial = ospNewMaterial2("scivis", "OBJMaterial");
      ospSet3f(objMaterial, "Ks", 0.5f, 0.5f, 0.5f);
      ospCommit(objMaterial);

      ospSetMaterial(geometry, objMaterial);
      ospRelease(objMaterial);

      ospCommit(geometry);

      OSPTestingGeometry retval;
      retval.geometry = geometry;
      retval.bounds   = reinterpret_cast<osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(SubdivisionCube, subdivision_cube);

  }  // namespace testing
}  // namespace ospray
