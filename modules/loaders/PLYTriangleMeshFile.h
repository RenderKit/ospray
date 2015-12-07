// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "modules/loaders/TriangleMeshFile.h"

//! \brief A concrete implementation of the TriangleMeshFile class
//!  for reading triangle data stored in the PLY file format on disk.
//!
class PLYTriangleMeshFile : public TriangleMeshFile {
public:

  //! Constructor.
  PLYTriangleMeshFile(const std::string &filename) : filename(filename), scale(osp::vec3f(1.f)), verbose(true) {}

  //! Destructor.
  virtual ~PLYTriangleMeshFile() {};

  //! Import the triangle data.
  virtual OSPTriangleMesh importTriangleMesh(OSPTriangleMesh triangleMesh);

  //! A string description of this class.
  virtual std::string toString() const { return("ospray_module_loaders::PLYTriangleMeshFile"); }

private:

  //! Path to the file containing the triangle data.
  std::string filename;

  //! Scaling for vertex coordinates.
  osp::vec3f scale;

  //! Verbose logging.
  bool verbose;

  //! Vertices.
  std::vector<osp::vec3fa> vertices;

  //! Vertex colors.
  std::vector<osp::vec4f> vertexColors;

  //! Vertex normals.
  std::vector<osp::vec3fa> vertexNormals;

  //! Triangle definitions.
  std::vector<osp::vec3i>  triangles;

  //! Parse the file, determining the vertices, vertex colors, and triangles.
  bool parse();

};
