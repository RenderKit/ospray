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

#include "PLYGeometryFile.h"
#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>

//! Error checking.
void exitOnCondition(bool condition, const std::string &message) { if (condition) throw std::runtime_error("PLYGeometryFile error: " + message + "."); }

//! String comparison helper.
bool startsWith(const std::string &haystack, const std::string &needle) {
  return needle.length() <= haystack.length() && equal(needle.begin(), needle.end(), haystack.begin());
}

PLYGeometryFile::PLYGeometryFile(const std::string &filename) : filename(filename) {

  //! Parse the file.
  exitOnCondition(parse() != true, "error parsing geometry file");
}

OSPTriangleMesh PLYGeometryFile::getOSPTriangleMesh() {

  OSPTriangleMesh triangleMesh = ospNewTriangleMesh();

  OSPData vertexData = ospNewData(vertices.size(), OSP_FLOAT3A, &vertices[0].x);
  ospSetData(triangleMesh, "vertex", vertexData);

  OSPData vertexColorData = ospNewData(vertexColors.size(), OSP_FLOAT3A, &vertexColors[0].x);
  ospSetData(triangleMesh, "vertex.color", vertexColorData);

  OSPData indexData = ospNewData(triangles.size(), OSP_INT3, &triangles[0].x);
  ospSetData(triangleMesh, "index", indexData);

  ospCommit(triangleMesh);

  return triangleMesh;
}

bool PLYGeometryFile::parse() {

  std::ifstream in(filename.c_str());
  exitOnCondition(!in.is_open(), "unable to open geometry file.");

  //! Verify header.
  char line[1024];
  in.getline(line, 1024);
  exitOnCondition(std::string(line) != "ply", "ply header not found.");

  in.getline(line, 1024);
  exitOnCondition(std::string(line) != "format ascii 1.0", "only ascii PLY files are supported.");

  //! Parse the rest of the header line by line.
  int numVertices = 0;
  int numVertexProperties = 0;
  std::map<std::string, int> vertexPropertyToIndex;
  int numFaces = 0;

  while(in.good()) {

    //! Read a new line.
    in.getline(line, 1024);
    std::string lineString(line);

    //! Ignore comment and obj_info lines.
    if(startsWith(lineString, "comment") || startsWith(lineString, "obj_info"))
      continue;

    //! Vertex header information.
    else if(startsWith(lineString, "element vertex")) {

      //! Number of vertices.
      exitOnCondition(sscanf(line, "element vertex %i", &numVertices) != 1, "could not read number of vertices.");

      //! Read vertex property definitions.
      while(in.good()) {
        std::streampos currentPos = in.tellg();

        in.getline(line, 1024);
        lineString = std::string(line);

        //! Seek backward and break after last property is found.
        if(!startsWith(lineString, "property")) {
          in.seekg(currentPos);
          break;
        }

        //! Read property name.
        char propertyType[64];
        char propertyName[64];
        exitOnCondition(sscanf(line, "property %s %s", propertyType, propertyName) != 2, "could not read vertex property definition.");

        //! Save property information.
        vertexPropertyToIndex[std::string(propertyName)] = numVertexProperties;
        numVertexProperties++;
      }
    }

    //! Face header information.
    else if(startsWith(lineString, "element face")) {

      //! Number of faces.
      exitOnCondition(sscanf(line, "element face %i", &numFaces) != 1, "could not read number of faces.");

      //! Next line should be "property list uchar int vertex_indices"
      in.getline(line, 1024);
      lineString = std::string(line);
      exitOnCondition(!startsWith(lineString, "property list uchar int vertex_indices"), "unexpected face header information.");
    }

    //! Break on end of header.
    else if(startsWith(lineString, "end_header"))
      break;

    //! Unexpected line.
    else
      exitOnCondition(true, "unexpected line in header: " + lineString);
  }

  std::cout << "numVertices = " << numVertices << ", numVertexProperties = " << numVertexProperties << ", numFaces = " << numFaces << std::endl;

  //! Make sure we have the required vertex properties.
  exitOnCondition(!vertexPropertyToIndex.count("x") || !vertexPropertyToIndex.count("y") || !vertexPropertyToIndex.count("z"), "vertex coordinate properties missing.");

  int xIndex = vertexPropertyToIndex["x"];
  int yIndex = vertexPropertyToIndex["y"];
  int zIndex = vertexPropertyToIndex["z"];
    
  //! See if we have vertex colors.
  bool haveVertexColors = false;
  int rIndex, gIndex, bIndex, aIndex;

  if(vertexPropertyToIndex.count("red") && vertexPropertyToIndex.count("green") && vertexPropertyToIndex.count("blue") && vertexPropertyToIndex.count("alpha")) {

    haveVertexColors = true;
    rIndex = vertexPropertyToIndex["red"];
    gIndex = vertexPropertyToIndex["green"];
    bIndex = vertexPropertyToIndex["blue"];
    aIndex = vertexPropertyToIndex["alpha"];
  }

  //! Read the vertex data.
  for(int i=0; i<numVertices; i++) {

    std::vector<float> vertexProperties;

    for(int j=0; j<numVertexProperties; j++) {
      float value;
      in >> value;
      exitOnCondition(!in.good(), "error reading vertex data.");

      vertexProperties.push_back(value);
    }

    vertices.push_back(osp::vec3fa(vertexProperties[xIndex], vertexProperties[yIndex], vertexProperties[zIndex]));

    //! Use vertex colors if we have them; otherwise default to white (note that the volume renderer currently requires a color for every vertex).
    if(haveVertexColors)
      vertexColors.push_back(1.f/255.f * osp::vec3fa(vertexProperties[rIndex], vertexProperties[gIndex], vertexProperties[bIndex]));
    else
      vertexColors.push_back(osp::vec3fa(1.f));
  }

  //! Read the face data.
  for(int i=0; i<numFaces; i++) {

    int count;
    in >> count;
    exitOnCondition(count != 3, "only triangle faces are supported.");

    osp::vec3i triangle;
    in >> triangle.x >> triangle.y >> triangle.z;
    exitOnCondition(!in.good(), "error reading face data.");

    triangles.push_back(triangle);
  }

  std::cout << "done." << std::endl;

  return true;
}
