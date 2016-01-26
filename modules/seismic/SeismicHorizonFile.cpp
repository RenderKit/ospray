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

#include <stdarg.h>
#include <vector>
#include <cstring>
#include "SeismicHorizonFile.h"

SeismicHorizonFile::SeismicHorizonFile(const std::string &filename) :
  filename(filename),
  scale(ospray::vec3f(1.f)),
  verbose(true)
{
}

OSPGeometry SeismicHorizonFile::importTriangleMesh(OSPGeometry triangleMesh)
{
  // Get scaling parameter if provided.
  ospGetVec3f(triangleMesh, "scale", (osp::vec3f *)&scale);

  // Open seismic data file and populate attributes.
  exitOnCondition(openSeismicDataFile(triangleMesh) != true,
                  "unable to open file '" + filename + "'");

  // Import the horizon data from the file into the triangle mesh.
  exitOnCondition(importHorizonData(triangleMesh) != true,
                  "error importing horizon data.");

  // Return the triangle mesh.
  return triangleMesh;
}

std::string SeismicHorizonFile::toString() const
{
  return("ospray_module_seismic::SeismicHorizonFile");
}

bool SeismicHorizonFile::openSeismicDataFile(OSPGeometry /*triangleMesh*/)
{
  // Open seismic data file, keeping trace header information. //
  // Sample types int, short, long, float, and double will be converted to float.
  inputBinTag = cddx_in2("in", filename.c_str(), "seismic");
  exitOnCondition(inputBinTag < 0, "could not open data file " + filename);

  // Get horizon volume dimensions.
  exitOnCondition(cdds_scanf("size.axis(1)", "%d", &dimensions.x) != 1,
                  "could not find size of dimension 1");
  exitOnCondition(cdds_scanf("size.axis(2)", "%d", &dimensions.y) != 1,
                  "could not find size of dimension 2");
  exitOnCondition(cdds_scanf("size.axis(3)", "%d", &dimensions.z) != 1,
                  "could not find size of dimension 3");

  if(verbose) {
    std::cout << toString() << " " << dimensions.z
              << " horizons, dimensions = " << dimensions.x
              << " " << dimensions.y << std::endl;
  }

  // Get voxel spacing (deltas).
  exitOnCondition(cdds_scanf("delta.axis(1)", "%f", &deltas.x) != 1,
                  "could not find delta of dimension 1");
  exitOnCondition(cdds_scanf("delta.axis(2)", "%f", &deltas.y) != 1,
                  "could not find delta of dimension 2");
  exitOnCondition(cdds_scanf("delta.axis(3)", "%f", &deltas.z) != 1,
                  "could not find delta of dimension 3");

  if(verbose) {
    std::cout << toString() << " volume deltas = "
              << deltas.x << " " << deltas.y << " " << deltas.z << std::endl;
  }

  // Get trace header information.
  FIELD_TAG traceSamplesTag = cdds_member(inputBinTag, 0, "Samples");
  traceHeaderSize = cdds_index(inputBinTag, traceSamplesTag, DDS_FLOAT);
  exitOnCondition(traceSamplesTag == -1 || traceHeaderSize == -1,
                  "could not get trace header information");

  if(verbose) {
    std::cout << toString() << " trace header size = " << traceHeaderSize
              << std::endl;
  }

  // Check that horizon dimensions are valid.
  exitOnCondition(dimensions.x <= 1 || dimensions.y <= 1,
                  "improper horizon dimensions found");

  // Need at least one horizon.
  exitOnCondition(dimensions.x < 1, "no horizons found");

  return true;
}

bool SeismicHorizonFile::importHorizonData(OSPGeometry triangleMesh)
{
  // Allocate memory for all traces.
  size_t memSize = dimensions.x*dimensions.y*dimensions.z * sizeof(float);
  float *volumeBuffer = (float *)malloc(memSize);
  exitOnCondition(volumeBuffer == NULL, "failed to allocate volume buffer");

  // Allocate trace array; the trace length is given by the trace header size
  // and the first dimension. Note that FreeDDS converts all trace data to
  // floats.
  memSize = (traceHeaderSize + dimensions.x) * sizeof(float);
  float *traceBuffer = (float *)malloc(memSize);
  exitOnCondition(traceBuffer == NULL, "failed to allocate trace buffer");

  // Read all traces and copy them into the volume buffer.
  for(long i3=0; i3<dimensions.z; i3++) {
    for(long i2=0; i2<dimensions.y; i2++) {

      // Read trace.
      exitOnCondition(cddx_read(inputBinTag, traceBuffer, 1) != 1,
                      "unable to read trace");

      // Copy trace into the volume buffer.
      memcpy((void *)&volumeBuffer[i3*dimensions.y*dimensions.x+i2*dimensions.x],
             (const void *) &traceBuffer[traceHeaderSize],
             dimensions.x * sizeof(float));
    }
  }

  // Generate triangle mesh for each horizon.
  std::vector<ospray::vec3fa> vertices;
  std::vector<ospray::vec3fa> vertexNormals;
  std::vector<ospray::vec3i>  triangles;

  for(long h=0; h<dimensions.z; h++) {

    // Generate vertices and vertex colors for this horizon.
    for(long i2=0; i2<dimensions.y; i2++) {
      for(long i1=0; i1<dimensions.x; i1++) {

        long index = h*dimensions.y*dimensions.x + i2*dimensions.x + i1;
        ospray::vec3fa vertex(volumeBuffer[index] * deltas.z, i1 * deltas.x,
                              i2 * deltas.y);

        // Apply scaling.
        vertex *= scale;

        vertices.push_back(vertex);
        vertexNormals.push_back(ospray::vec3fa(0.f));
      }
    }

    // Generate triangles and vertex normals for this horizon.
    for(long i2=0; i2<dimensions.y-1; i2++) {
      for(long i1=0; i1<dimensions.x-1; i1++) {

        long v0 = h*dimensions.x*dimensions.y + i2    *dimensions.x + i1    ;
        long v1 = h*dimensions.x*dimensions.y + i2    *dimensions.x + (i1+1);
        long v2 = h*dimensions.x*dimensions.y + (i2+1)*dimensions.x + (i1+1);
        long v3 = h*dimensions.x*dimensions.y + (i2+1)*dimensions.x + i1    ;

        // Assume horizon height coordinate must be > 0 to be valid.
        if (std::min(std::min(vertices[v0].x,
                              vertices[v1].x),
                     vertices[v2].x) > 0.f) {
          triangles.push_back(ospray::vec3i(v0, v1, v2));

          ospray::vec3fa triangleNormal = cross(vertices[v1] - vertices[v0],
                                                vertices[v2] - vertices[v0]);
          vNormals[v0] += triangleNormal;
          vNormals[v1] += triangleNormal;
          vNormals[v2] += triangleNormal;
        }

        if (std::min(std::min(vertices[v2].x,
                              vertices[v3].x),
                     vertices[v0].x) > 0.f) {
          triangles.push_back(ospray::vec3i(v2, v3, v0));

          ospray::vec3fa triangleNormal = cross(vertices[v3] - vertices[v2],
                                                vertices[v0] - vertices[v2]);
          vNormals[v2] += triangleNormal;
          vNormals[v3] += triangleNormal;
          vNormals[v0] += triangleNormal;
        }
      }
    }
  }

  // Normalize vertex normals.
  for(long i=0; i<vertexNormals.size(); i++)
    vertexNormals[i] = normalize(vertexNormals[i]);

  // Set data on triangle mesh.
  OSPData vertexData = ospNewData(vertices.size(), OSP_FLOAT3A, &vertices[0].x);
  ospSetData(triangleMesh, "vertex", vertexData);

  OSPData vertexNormalData = ospNewData(vertexNormals.size(),
                                        OSP_FLOAT3A,
                                        &vertexNormals[0].x);
  ospSetData(triangleMesh, "vertex.normal", vertexNormalData);

  OSPData indexData = ospNewData(triangles.size(), OSP_INT3, &triangles[0].x);
  ospSetData(triangleMesh, "index", indexData);

  // Clean up.
  free(volumeBuffer);
  free(traceBuffer);

  // Close the seismic data file.
  cdds_close(inputBinTag);

  return true;
}
