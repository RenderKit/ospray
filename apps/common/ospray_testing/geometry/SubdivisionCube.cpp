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

      std::vector<vec3f> vertices = {{-1.0f, -1.0f, -1.0f},
                                     {1.0f, -1.0f, -1.0f},
                                     {1.0f, -1.0f, 1.0f},
                                     {-1.0f, -1.0f, 1.0f},
                                     {-1.0f, 1.0f, -1.0f},
                                     {1.0f, 1.0f, -1.0f},
                                     {1.0f, 1.0f, 1.0f},
                                     {-1.0f, 1.0f, 1.0f}};

      std::vector<vec4f> colors = {{0.0f, 0.0f, 0.0f, 1.f},
                                   {1.0f, 0.0f, 0.0f, 1.f},
                                   {1.0f, 0.0f, 1.0f, 1.f},
                                   {0.0f, 0.0f, 1.0f, 1.f},
                                   {0.0f, 1.0f, 0.0f, 1.f},
                                   {1.0f, 1.0f, 0.0f, 1.f},
                                   {1.0f, 1.0f, 1.0f, 1.f},
                                   {0.0f, 1.0f, 1.0f, 1.f}};

      std::vector<unsigned int> indices = {
          0, 4, 5, 1, 1, 5, 6, 2, 2, 6, 7, 3,
          0, 3, 7, 4, 4, 7, 6, 5, 0, 1, 2, 3,
      };

      std::vector<unsigned int> faces = {4, 4, 4, 4, 4, 4};

      std::vector<float> vertexCreaseWeights = {2, 2, 2, 2, 2, 2, 2, 2};

      std::vector<unsigned int> vertexCreaseIndices = {0, 1, 2, 3, 4, 5, 6, 7};

      std::vector<float> edgeCreaseWeights = {
          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

      std::vector<unsigned int> edgeCreaseIndices = {
          0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
          6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7,
      };

      OSPGeometry geometry = ospNewGeometry("subdivision");

      OSPData verticesData = ospNewData(8, OSP_FLOAT3, vertices.data());
      ospSetData(geometry, "vertex", verticesData);
      ospRelease(verticesData);

      OSPData indicesData =
          ospNewData(indices.size(), OSP_UINT, indices.data());
      ospSetData(geometry, "index", indicesData);
      ospRelease(indicesData);

      OSPData facesData = ospNewData(faces.size(), OSP_UINT, faces.data());
      ospSetData(geometry, "face", facesData);
      ospRelease(facesData);

      OSPData edgeCreaseIndicesData =
          ospNewData(12, OSP_UINT2, edgeCreaseIndices.data());
      ospSetData(geometry, "edgeCrease.index", edgeCreaseIndicesData);
      ospRelease(edgeCreaseIndicesData);

      OSPData edgeCreaseWeightsData =
          ospNewData(12, OSP_FLOAT, edgeCreaseWeights.data());
      ospSetData(geometry, "edgeCrease.weight", edgeCreaseWeightsData);
      ospRelease(edgeCreaseWeightsData);

      OSPData vertexCreaseIndicesData =
          ospNewData(8, OSP_UINT, vertexCreaseIndices.data());
      ospSetData(geometry, "vertexCrease.index", vertexCreaseIndicesData);
      ospRelease(vertexCreaseIndicesData);

      OSPData vertexCreaseWeightsData =
          ospNewData(8, OSP_FLOAT, vertexCreaseWeights.data());
      ospSetData(geometry, "vertexCrease.weight", vertexCreaseWeightsData);
      ospRelease(vertexCreaseWeightsData);

      OSPData colorsData = ospNewData(8, OSP_FLOAT4, colors.data());
      ospSetData(geometry, "color", colorsData);
      ospRelease(colorsData);

      ospSet1f(geometry, "level", 5.0f);

      // create OBJ material and assign to geometry
      OSPMaterial objMaterial = ospNewMaterial2("scivis", "OBJMaterial");
      ospSet3f(objMaterial, "Ks", 0.5f, 0.5f, 0.5f);
      ospCommit(objMaterial);

      ospSetMaterial(geometry, objMaterial);

      ospCommit(geometry);

      OSPTestingGeometry retval;
      retval.geometry = geometry;
      retval.bounds   = reinterpret_cast<osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(SubdivisionCube, subdivision_cube);

  }  // namespace testing
}  // namespace ospray
