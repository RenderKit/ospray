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

#undef NDEBUG

// ospray
#include "StreamLines.h"
#include "common/Data.h"
#include "common/Model.h"
// ispc-generated files
#include "StreamLines_ispc.h"

namespace ospray {

  StreamLines::StreamLines()
  {
    this->ispcEquivalent = ispc::StreamLineGeometry_create(this);
  }

  void StreamLines::finalize(Model *model) 
  {
    radius     = getParam1f("radius",0.01f);
    vertexData = getParamData("vertex",NULL);
    indexData  = getParamData("index",NULL);
    colorData  = getParamData("vertex.color",getParamData("color"));

    Assert(radius > 0.f);
    Assert(vertexData);
    Assert(indexData);

    index       = (const uint32*)indexData->data;
    numSegments = indexData->numItems;
    vertex      = (const vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;
    color       = colorData ? (const vec4f*)colorData->data : NULL;

    std::cout << "#osp: creating streamlines geometry, "
              << "#verts=" << numVertices << ", "
              << "#segments=" << numSegments << ", "
              << "radius=" << radius << std::endl;
    
    ispc::StreamLineGeometry_set(getIE(),model->getIE(),radius,
                                 (ispc::vec3fa*)vertex,numVertices,
                                 (uint32_t*)index,numSegments,
                                 (ispc::vec4f*)color);
  }

  OSP_REGISTER_GEOMETRY(StreamLines,streamlines);

} // ::ospray
