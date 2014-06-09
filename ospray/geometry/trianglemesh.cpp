#include "trianglemesh.h"
#include "../common/model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
#include "../include/ospray/ospray.h"
// ispc side
#include "trianglemesh_ispc.h"

#define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID

namespace ospray {
  using std::cout;
  using std::endl;

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
    if (logLevel > 2) 
      if (numPrints == 5)
        cout << "(all future printouts for triangle mesh creation will be emitted)" << endl;
    
    if (logLevel > 2) 
      if (numPrints < 5)
        std::cout << "ospray: finalizing triangle mesh ..." << std::endl;
    Assert((eMesh == RTC_INVALID_ID) && "triangle mesh already built!?");
    
    Assert(model && "invalid model pointer");
    RTCScene embreeSceneHandle = model->embreeSceneHandle;

    vertexData = getParamData("vertex",getParamData("position"));
    normalData = getParamData("vertex.normal",getParamData("normal"));
    colorData  = getParamData("vertex.color",getParamData("color"));
    indexData  = getParamData("index",getParamData("triangle"));
    materialIDData = getParamData("prim.materialID");
    primMaterialList = getParamData("materials");
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
    this->prim_materialID  = materialIDData ? (uint32*)materialIDData->data : NULL;
    this->prim_material_list = primMaterialList ? (Material**)primMaterialList->data : NULL;
    //Get ISPC pointers
    const int num_materials = primMaterialList->numItems;
    ispcMaterialPtrs = new void*[num_materials];
    for (int i = 0; i < num_materials; i++) {
      this->ispcMaterialPtrs[i] = this->prim_material_list[i]->getIE();
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
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_VERTEX_BUFFER,
                 (void*)this->vertex,0,
                 ospray::sizeOf(vertexData->type));
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_INDEX_BUFFER,
                 (void*)this->index,0,
                 ospray::sizeOf(indexData->type));

    box3f bounds = embree::empty;
    
    for (int i=0;i<vertexData->size();i++) 
      bounds.extend(vertex[i]);

    if (logLevel > 2) 
      if (numPrints < 5) {
        cout << "  created triangle mesh (" << numTris << " tris "
             << ", " << numVerts << " vertices)" << endl;
        cout << "  mesh bounds " << bounds << endl;
      } 
    rtcEnable(model->embreeSceneHandle,eMesh);

    ispc::TriangleMesh_set(getIE(),model->getIE(),eMesh,
                           numTris,
                           (ispc::vec3i*)index,
                           (ispc::vec3fa*)vertex,
                           (ispc::vec3fa*)normal,
                           (ispc::vec4f*)color,
                           geom_materialID,
                           &ispcMaterialPtrs[0],
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
    mesh->findParam("position",1)->set(new Data(vtx.size(),OSP_vec3fa,&vtx[0],0));
    // mesh->commit();
    return mesh;
  }
}
