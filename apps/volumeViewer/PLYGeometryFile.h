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
