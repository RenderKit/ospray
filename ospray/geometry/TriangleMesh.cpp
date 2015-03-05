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

// ospray
#include "TriangleMesh.h"
#include "ospray/common/Model.h"
#include "../include/ospray/ospray.h"
// embree 
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
// ispc exports
#include "TriangleMesh_ispc.h"

#define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID

namespace ospray {
  using std::cout;
  using std::endl;

  inline bool inRange(int64 i, int64 i0, int64 i1)
  {
    return i >= i0 && i < i1;
  }

  TriangleMesh::TriangleMesh() 
    : eMesh(RTC_INVALID_ID)
  {
    this->ispcEquivalent = ispc::TriangleMesh_create(this);
  }


  void TriangleMesh::finalize(Model *model)
  {
    static int numPrints = 0;
    numPrints++;
    if (logLevel >= 2) 
      if (numPrints == 5)
        cout << "(all future printouts for triangle mesh creation will be emitted)" << endl;
    
    if (logLevel >= 2) 
      if (numPrints < 5)
        std::cout << "ospray: finalizing triangle mesh ..." << std::endl;
    Assert((eMesh == RTC_INVALID_ID) && "triangle mesh already built!?");
    
    Assert(model && "invalid model pointer");

    RTCScene embreeSceneHandle = model->embreeSceneHandle;

    vertexData = getParamData("vertex",getParamData("position"));
    normalData = getParamData("vertex.normal",getParamData("normal"));
    colorData  = getParamData("vertex.color",getParamData("color"));
    texcoordData = getParamData("vertex.texcoord",getParamData("texcoord"));
    indexData  = getParamData("index",getParamData("triangle"));
    prim_materialIDData = getParamData("prim.materialID");
    materialListData = getParamData("materialList");
    geom_materialID = getParam1i("geom.materialID",-1);

    Assert2(vertexData != NULL,
            "triangle mesh geometry does not have either 'position'"
            " or 'vertex' array");
    Assert2(indexData != NULL, 
            "triangle mesh geometry does not have either 'index'"
            " or 'triangle' array");

    this->index = (vec3i*)indexData->data;
    this->vertex = (vec3fa*)vertexData->data;
    this->normal = normalData ? (vec3fa*)normalData->data : NULL;
    this->color  = colorData ? (vec4f*)colorData->data : NULL;
    this->texcoord = texcoordData ? (vec2f*)texcoordData->data : NULL;
    this->prim_materialID  = prim_materialIDData ? (uint32*)prim_materialIDData->data : NULL;
    this->materialList  = materialListData ? (ospray::Material**)materialListData->data : NULL;

    if (materialList) {
      const int num_materials = materialListData->numItems;
      ispcMaterialPtrs = new void*[num_materials];
      for (int i = 0; i < num_materials; i++) {
        this->ispcMaterialPtrs[i] = this->materialList[i]->getIE();
      }
    } else {
      this->ispcMaterialPtrs = NULL;
    }

    size_t numTris  = -1;
    size_t numVerts = -1;

    size_t numCompsInTri = 0;
    size_t numCompsInVtx = 0;
    switch (indexData->type) {
    case OSP_INT:   numTris = indexData->size() / 3; numCompsInTri = 3; break;
    case OSP_UINT:  numTris = indexData->size() / 3; numCompsInTri = 3; break;
    case OSP_INT3:  numTris = indexData->size(); numCompsInTri = 3; break;
    case OSP_UINT3: numTris = indexData->size(); numCompsInTri = 3; break;
    case OSP_INT4:  numTris = indexData->size(); numCompsInTri = 4; break;
    default:
      throw std::runtime_error("unsupported trianglemesh.index data type");
    }
    switch (vertexData->type) {
    case OSP_FLOAT:   numVerts = vertexData->size() / 4; numCompsInVtx = 4; break;
    case OSP_FLOAT3:  numVerts = vertexData->size(); numCompsInVtx = 3; break;
    case OSP_FLOAT3A: numVerts = vertexData->size(); numCompsInVtx = 4; break;
    default:
      throw std::runtime_error("unsupported trianglemesh.vertex data type");
    }

    eMesh = rtcNewTriangleMesh(embreeSceneHandle,RTC_GEOMETRY_STATIC,
                               numTris,numVerts);
// #ifndef NDEBUG
//     {
//       cout << "#osp/trimesh: Verifying index buffer ... " << endl;
//       if ((sizeOf(indexData->type) == 4*sizeof(int)
//       for (int i=0;i<numTris;i++) {
//              &&  (!inRange(index4i[i].x,0,numVerts) || 
//                   !inRange(index4i[i].y,0,numVerts) || 
//                   !inRange(index4i[i].z,0,numVerts)))
//             ||
//             (sizeOf(indexData->type) == 4*sizeof(int) 
//              &&  (!inRange(index3i[i].x,0,numVerts) || 
//                   !inRange(index3i[i].y,0,numVerts) || 
//                   !inRange(index3i[i].z,0,numVerts)))
//             )
//           {
//           PRINT(numTris);
//           PRINT(i);
//           PRINT(index[i]);
//           PRINT(numVerts);
//           throw std::runtime_error("vertex index not in range! (broken input model, refusing to handle that)");
//         }
//       }
//       cout << "#osp/trimesh: Verifying vertex buffer ... " << endl;
//       for (int i=0;i<numVerts;i++) {
//         if (std::isnan(vertex[i].x) || 
//             std::isnan(vertex[i].y) || 
//             std::isnan(vertex[i].z))
//           throw std::runtime_error("NaN in vertex coordinate! (broken input model, refusing to handle that)");
//       }
//     }    
// #endif

    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_VERTEX_BUFFER,
                 (void*)this->vertex,0,
                 sizeOf(vertexData->type));
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_INDEX_BUFFER,
                 (void*)this->index,0,
                 sizeOf(indexData->type));

    bounds = embree::empty;
    
    for (int i=0;i<vertexData->size();i++) 
      bounds.extend(vertex[i]);

    if (logLevel >= 2) 
      if (numPrints < 5) {
        cout << "  created triangle mesh (" << numTris << " tris "
             << ", " << numVerts << " vertices)" << endl;
        cout << "  mesh bounds " << bounds << endl;
      } 

    PRINT(numCompsInVtx);
    PRINT(numCompsInTri);
    if (numCompsInVtx==3 && numCompsInTri==4 ||
        numCompsInVtx==4 && numCompsInTri==3)
      ispc::TriangleMesh_set(getIE(),model->getIE(),eMesh,
                           numTris,
                           numCompsInTri,
                           numCompsInVtx,
                             (int*)index,
                             (float*)vertex,
                             (float*)normal,
                           (ispc::vec4f*)color,
                           (ispc::vec2f*)texcoord,
                           geom_materialID,
                           getMaterial()?getMaterial()->getIE():NULL,
                           ispcMaterialPtrs,
                           (uint32*)prim_materialID);
    else
      throw std::runtime_error("#osp:trianglemesh: vertexsize/trianglesize variant not implemented");
  }

} // ::ospray
