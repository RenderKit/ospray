// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#pragma once

#include <string>
#include <vector>
#include <ospray/ospray.h>

class PLYGeometryFile {

public:

  PLYGeometryFile(const std::string &filename);

  //! Get an OSPTriangleMesh representing the geometry.
  OSPTriangleMesh getOSPTriangleMesh();

protected:

  //! PLY geometry filename.
  std::string filename;

  //! Vertices.
  std::vector<osp::vec3fa> vertices;

  //! Vertex colors.
  std::vector<osp::vec3fa> vertexColors;

  //! Triangle definitions.
  std::vector<osp::vec3i>  triangles;

  //! Parse the file, determining the vertices, vertex colors, and triangles.
  bool parse();

};
