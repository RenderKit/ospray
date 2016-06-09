// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "PLYTriangleMeshFile.h"
#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>

//! String comparison helper.
bool startsWith(const std::string &haystack, const std::string &needle)
{
  return needle.length() <= haystack.length() &&
         equal(needle.begin(), needle.end(), haystack.begin());
}

PLYTriangleMeshFile::PLYTriangleMeshFile(const std::string &filename) :
  filename(filename),
  scale(ospcommon::vec3f(1.f)),
  verbose(true)
{
}

OSPGeometry PLYTriangleMeshFile::importTriangleMesh(OSPGeometry triangleMesh)
{
  // Get scaling parameter if provided.
  ospGetVec3f(triangleMesh, "scale", (osp::vec3f*)&scale);

  // Parse the PLY triangle data file and populate attributes.
  exitOnCondition(parse() != true, "error parsing the file '" + filename + "'");

  // Set the vertex, vertex colors and triangle index data on the triangle mesh.
  OSPData vertexData = ospNewData(vertices.size(), OSP_FLOAT3A, &vertices[0].x);
  ospSetData(triangleMesh, "vertex", vertexData);

  OSPData vertexColorData = ospNewData(vertexColors.size(), OSP_FLOAT4, &vertexColors[0].x);
  ospSetData(triangleMesh, "vertex.color", vertexColorData);

  OSPData vertexNormalData = ospNewData(vertexNormals.size(), OSP_FLOAT3A, &vertexNormals[0].x);
  ospSetData(triangleMesh, "vertex.normal", vertexNormalData);

  OSPData indexData = ospNewData(triangles.size(), OSP_INT3, &triangles[0].x);
  ospSetData(triangleMesh, "index", indexData);

  // Return the triangle mesh.
  return triangleMesh;
}

std::string PLYTriangleMeshFile::toString() const
{
  return("ospray_module_loaders::PLYTriangleMeshFile");
}

bool PLYTriangleMeshFile::parse()
{
  std::ifstream in(filename.c_str());
  exitOnCondition(!in.is_open(), "unable to open geometry file.");

  // Verify header.
  char line[1024];
  in.getline(line, 1024);
  exitOnCondition(std::string(line) != "ply", "ply header not found.");

  in.getline(line, 1024);
  exitOnCondition(std::string(line) != "format ascii 1.0", "only ascii PLY files are supported.");

  // Parse the rest of the header line by line.
  int numVertices = 0;
  int numVertexProperties = 0;
  std::map<std::string, int> vertexPropertyToIndex;
  int numFaces = 0;

  while(in.good()) {

    // Read a new line.
    in.getline(line, 1024);
    std::string lineString(line);

    // Ignore comment and obj_info lines.
    if(startsWith(lineString, "comment") || startsWith(lineString, "obj_info"))
      continue;

    // Vertex header information.
    else if(startsWith(lineString, "element vertex")) {

      // Number of vertices.
      exitOnCondition(sscanf(line, "element vertex %i", &numVertices) != 1, "could not read number of vertices.");

      // Read vertex property definitions.
      while(in.good()) {
        std::streampos currentPos = in.tellg();

        in.getline(line, 1024);
        lineString = std::string(line);

        // Seek backward and break after last property is found.
        if(!startsWith(lineString, "property")) {
          in.seekg(currentPos);
          break;
        }

        // Read property name.
        char propertyType[64];
        char propertyName[64];
        exitOnCondition(sscanf(line, "property %s %s", propertyType, propertyName) != 2, "could not read vertex property definition.");

        // Save property information.
        vertexPropertyToIndex[std::string(propertyName)] = numVertexProperties;
        numVertexProperties++;
      }
    }

    // Face header information.
    else if(startsWith(lineString, "element face")) {

      // Number of faces.
      exitOnCondition(sscanf(line, "element face %i", &numFaces) != 1, "could not read number of faces.");

      // Next line should be "property list uchar int vertex_indices"
      in.getline(line, 1024);
      lineString = std::string(line);
      exitOnCondition(!startsWith(lineString, "property list uchar int vertex_indices"), "unexpected face header information.");
    }

    // Break on end of header.
    else if(startsWith(lineString, "end_header"))
      break;

    // Unexpected line.
    else
      exitOnCondition(true, "unexpected line in header: " + lineString);
  }

  if(verbose)
    std::cout << toString() << " numVertices = " << numVertices << ", numVertexProperties = " << numVertexProperties << ", numFaces = " << numFaces << std::endl;

  // Make sure we have the required vertex properties.
  exitOnCondition(!vertexPropertyToIndex.count("x") || !vertexPropertyToIndex.count("y") || !vertexPropertyToIndex.count("z"), "vertex coordinate properties missing.");

  int xIndex = vertexPropertyToIndex["x"];
  int yIndex = vertexPropertyToIndex["y"];
  int zIndex = vertexPropertyToIndex["z"];
    
  // See if we have vertex colors.
  bool haveVertexColors = false;
  int rIndex, gIndex, bIndex, aIndex;

  if(vertexPropertyToIndex.count("red") && vertexPropertyToIndex.count("green") && vertexPropertyToIndex.count("blue") && vertexPropertyToIndex.count("alpha")) {

    haveVertexColors = true;
    rIndex = vertexPropertyToIndex["red"];
    gIndex = vertexPropertyToIndex["green"];
    bIndex = vertexPropertyToIndex["blue"];
    aIndex = vertexPropertyToIndex["alpha"];
  }

  // Read the vertex data.
  for(int i=0; i<numVertices; i++) {

    std::vector<float> vertexProperties;

    for(int j=0; j<numVertexProperties; j++) {
      float value;
      in >> value;
      exitOnCondition(!in.good(), "error reading vertex data.");

      vertexProperties.push_back(value);
    }

    // Add to vertices vector with scaling applied.
    vertices.push_back(scale * ospcommon::vec3fa(vertexProperties[xIndex], vertexProperties[yIndex], vertexProperties[zIndex]));

    // Vertex normals will be computed later.
    vertexNormals.push_back(ospcommon::vec3fa(0.f));

    // Use vertex colors if we have them; otherwise default to white (note that the volume renderer currently requires a color for every vertex).
    if(haveVertexColors)
      vertexColors.push_back(1.f/255.f * ospcommon::vec4f(vertexProperties[rIndex], vertexProperties[gIndex], vertexProperties[bIndex], vertexProperties[aIndex]));
    else
      vertexColors.push_back(ospcommon::vec4f(1.f));
  }

  // Read the face data.
  for(int i=0; i<numFaces; i++) {

    int count;
    in >> count;
    exitOnCondition(count != 3, "only triangle faces are supported.");

    ospcommon::vec3i triangle;
    in >> triangle.x >> triangle.y >> triangle.z;
    exitOnCondition(!in.good(), "error reading face data.");

    triangles.push_back(triangle);

    // Add vertex normal contributions.
    ospcommon::vec3fa triangleNormal = cross(vertices[triangle.y] - vertices[triangle.x], vertices[triangle.z] - vertices[triangle.x]);
    vertexNormals[triangle.x] += triangleNormal;
    vertexNormals[triangle.y] += triangleNormal;
    vertexNormals[triangle.z] += triangleNormal;
  }

  // Normalize vertex normals.
  for(int i=0; i<vertexNormals.size(); i++)
    vertexNormals[i] = normalize(vertexNormals[i]);

  if(verbose)
    std::cout << toString() << " done." << std::endl;

  return true;
}
