/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "trianglemesh.h"
#include "../common/model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
#include "../include/ospray/ospray.h"
// ispc side
#include "trianglemesh_ispc.h"
// C
// #include <math.h>

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
    //    cout << "creating new ispc-side trianglemesh!" << endl;
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
    explosion_factor = model->getParam1f("explosion.factor", 0.f);

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
    switch (indexData->type) {
    case OSP_int32:  numTris = indexData->size() / 3; break;
    case OSP_uint32: numTris = indexData->size() / 3; break;
    case OSP_vec3i:  numTris = indexData->size(); break;
    case OSP_vec3ui: numTris = indexData->size(); break;
    default:
      throw std::runtime_error("unsupported trianglemesh.index data type");
    }
    switch (vertexData->type) {
    case OSP_float:   numVerts = vertexData->size() / 4; break;
    case OSP_vec3fa:  numVerts = vertexData->size(); break;
    default:
      throw std::runtime_error("unsupported trianglemesh.vertex data type");
    }

    eMesh = rtcNewTriangleMesh(embreeSceneHandle,RTC_GEOMETRY_STATIC,
                               numTris,numVerts);
#ifndef NDEBUG
    {
      cout << "#osp/trimesh: Verifying index buffer ... " << endl;
      for (int i=0;i<numTris;i++) {
        if (!inRange(index[i].x,0,numVerts) || 
            !inRange(index[i].y,0,numVerts) || 
            !inRange(index[i].z,0,numVerts))
          throw std::runtime_error("vertex index not in range! (broken input model, refusing to handle that)");
      }
      cout << "#osp/trimesh: Verifying vertex buffer ... " << endl;
      for (int i=0;i<numVerts;i++) {
        if (std::isnan(vertex[i].x) || 
            std::isnan(vertex[i].y) || 
            std::isnan(vertex[i].z))
          throw std::runtime_error("NaN in vertex coordinate! (broken input model, refusing to handle that)");
      }
    }    
#endif

    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_VERTEX_BUFFER,
                 (void*)this->vertex,0,
                 ospray::sizeOf(vertexData->type));
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_INDEX_BUFFER,
                 (void*)this->index,0,
                 ospray::sizeOf(indexData->type));

    box3f bounds = embree::empty;
    
    for (int i=0;i<vertexData->size();i++) 
      bounds.extend(vertex[i]);

    if (logLevel >= 2) 
      if (numPrints < 5) {
        cout << "  created triangle mesh (" << numTris << " tris "
             << ", " << numVerts << " vertices)" << endl;
        cout << "  mesh bounds " << bounds << endl;
      } 

    ispc::TriangleMesh_set(getIE(),model->getIE(),eMesh,
                           numTris,
                           (ispc::vec3i*)index,
                           (ispc::vec3fa*)vertex,
                           (ispc::vec3fa*)normal,
                           (ispc::vec4f*)color,
                           (ispc::vec2f*)texcoord,
                           geom_materialID,
                           ispcMaterialPtrs,
                           (uint32*)prim_materialID);
  }

  //! helper fct that creates a tessllated unit arrow
  /*! this function creates a tessllated 'unit' arrow, where 'unit'
    means itreaches from Z=-1 to Z=+1. With of arrow head and
    arrow body are given as parameters, as it the number of
    segments for tessellating. The total number of triangles from
    this arrow is 'numSegments*5'. */
  ospray::TriangleMesh *makeArrow(int numSegments,
                                  float headWidth,
                                  float bodyWidth,
                                  float headLength)
  {
    std::vector<vec3fa> vtx;
    std::vector<vec3i>  idx;

    // tail ring
    int tailCenter = vtx.size();
    vtx.push_back(vec3fa(0,0,-1.f));
    int tailRingStart = vtx.size();
    for (int i=0;i<numSegments;i++) {
      const float u = cosf(i*(2.f*M_PI/numSegments));
      const float v = sinf(i*(2.f*M_PI/numSegments));
      vtx.push_back(vec3fa(u*bodyWidth,v*bodyWidth,-1.f));
      idx.push_back(vec3i(tailCenter,
                          tailRingStart+i,
                          tailRingStart+((i+1)%numSegments)));
    }
#if 1
    // body
    int bodyRingStart = vtx.size();
    for (int i=0;i<numSegments;i++) {
      const float u = cosf(i*(2.f*M_PI/numSegments));
      const float v = sinf(i*(2.f*M_PI/numSegments));
      vtx.push_back(vec3fa(u*bodyWidth,v*bodyWidth,+1.f-headLength));
      int a = tailRingStart+i;
      int b = bodyRingStart+i;
      int c = tailRingStart+(i+1)%numSegments;
      int d = bodyRingStart+(i+1)%numSegments;
      idx.push_back(vec3i(a,b,c));
      idx.push_back(vec3i(b,c,d));
    }

    // tip 
    int tipTop = vtx.size();
    vtx.push_back(vec3fa(0,0,+1.f));
    int tipMid = vtx.size();
    vtx.push_back(vec3fa(0,0,+1.f-headLength));
    int tipRingStart = vtx.size();
    for (int i=0;i<numSegments;i++) {
      const float u = cosf(i*(2.f*M_PI/numSegments));
      const float v = sinf(i*(2.f*M_PI/numSegments));
      vtx.push_back(vec3fa(u*headWidth,v*headWidth,+1.f-headLength));
      int a = tipRingStart+i;
      int b = tipRingStart+(i+1)%numSegments;
      idx.push_back(vec3i(a,b,tipTop));
      idx.push_back(vec3i(a,b,tipMid));
    }
#endif

    ospray::TriangleMesh *mesh = new TriangleMesh;
    mesh->findParam("index",1)->set(new Data(idx.size(),OSP_vec3i,&idx[0],0));
    mesh->findParam("vertex",1)->set(new Data(vtx.size(),OSP_vec3fa,&vtx[0],0));
    // mesh->commit();
    return mesh;
  }
}
