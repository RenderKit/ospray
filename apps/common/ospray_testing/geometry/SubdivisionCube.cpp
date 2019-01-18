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
      box3f cube_bounds(-1.f, 1.f);

      float cube_vertices[8][3] = {{-1.0f, -1.0f, -1.0f},
                                   {1.0f, -1.0f, -1.0f},
                                   {1.0f, -1.0f, 1.0f},
                                   {-1.0f, -1.0f, 1.0f},
                                   {-1.0f, 1.0f, -1.0f},
                                   {1.0f, 1.0f, -1.0f},
                                   {1.0f, 1.0f, 1.0f},
                                   {-1.0f, 1.0f, 1.0f}};

      float cube_colors[8][4] = {{0.0f, 0.0f, 0.0f, 1.f},
                                 {1.0f, 0.0f, 0.0f, 1.f},
                                 {1.0f, 0.0f, 1.0f, 1.f},
                                 {0.0f, 0.0f, 1.0f, 1.f},
                                 {0.0f, 1.0f, 0.0f, 1.f},
                                 {1.0f, 1.0f, 0.0f, 1.f},
                                 {1.0f, 1.0f, 1.0f, 1.f},
                                 {0.0f, 1.0f, 1.0f, 1.f}};

      unsigned int cube_indices[24] = {
          0, 4, 5, 1, 1, 5, 6, 2, 2, 6, 7, 3,
          0, 3, 7, 4, 4, 7, 6, 5, 0, 1, 2, 3,
      };

      unsigned int cube_faces[6] = {4, 4, 4, 4, 4, 4};

      float cube_vertex_crease_weights[8] = {2, 2, 2, 2, 2, 2, 2, 2};

      unsigned int cube_vertex_crease_indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};

      float cube_edge_crease_weights[12] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

      unsigned int cube_edge_crease_indices[24] = {
          0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
          6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7,
      };

      int numIndices = 24;
      int numFaces   = 6;
      int faceSize   = 4;

      auto subd     = ospNewGeometry("subdivision");
      auto vertices = ospNewData(8, OSP_FLOAT3, cube_vertices);
      ospSetData(subd, "vertex", vertices);
      auto indices = ospNewData(numIndices, OSP_UINT, cube_indices);
      ospSetData(subd, "index", indices);
      auto faces = ospNewData(numFaces, OSP_UINT, cube_faces);
      ospSetData(subd, "face", faces);
      auto edge_crease_indices =
          ospNewData(12, OSP_UINT2, cube_edge_crease_indices);
      ospSetData(subd, "edgeCrease.index", edge_crease_indices);
      auto edge_crease_weights =
          ospNewData(12, OSP_FLOAT, cube_edge_crease_weights);
      ospSetData(subd, "edgeCrease.weight", edge_crease_weights);
      auto vertex_crease_indices =
          ospNewData(8, OSP_UINT, cube_vertex_crease_indices);
      ospSetData(subd, "vertexCrease.index", vertex_crease_indices);
      auto vertex_crease_weights =
          ospNewData(8, OSP_FLOAT, cube_vertex_crease_weights);
      ospSetData(subd, "vertexCrease.weight", vertex_crease_weights);
      auto colors = ospNewData(8, OSP_FLOAT4, cube_colors);
      ospSetData(subd, "color", colors);
      ospSet1f(subd, "level", 128.0f);

      // create OBJ material and assign to geometry
      OSPMaterial objMaterial = ospNewMaterial2("scivis", "OBJMaterial");
      ospSet3f(objMaterial, "Ks", 0.5f, 0.5f, 0.5f);
      ospCommit(objMaterial);

      ospSetMaterial(subd, objMaterial);

      ospCommit(subd);

      OSPTestingGeometry retval;
      retval.geometry = subd;
      retval.bounds   = reinterpret_cast<osp::box3f &>(cube_bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(SubdivisionCube, subdivision_cube);

  }  // namespace testing
}  // namespace ospray
