#include "trianglemesh.h"
#include "../common/model.h"
// embree stuff
#include "rtcore.h"
#include "rtcore_scene.h"
#include "rtcore_geometry.h"
#include "../include/ospray/ospray.h"

#define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID

namespace ospray {
  using std::cout;
  using std::endl;

  TriangleMesh::TriangleMesh() 
    : eMesh(RTC_INVALID_ID)
  {}

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
    RTCScene eScene = model->eScene;

    Param *posParam = findParam("position");
    Assert(posParam != NULL && "triangle mesh geometry does not have a 'position' array");
    posData = dynamic_cast<Data*>(posParam->ptr);
    Assert(posData != NULL && "invalid position array");

    Param *idxParam = findParam("index");
    Assert(idxParam != NULL && "triangle mesh geometry does not have a 'index' array");
    idxData = dynamic_cast<Data*>(idxParam->ptr);
    Assert(idxData != NULL && "invalid index array");

    size_t numTris  = idxData->size();
    size_t numVerts = posData->size();

    eMesh = rtcNewTriangleMesh(eScene,RTC_GEOMETRY_STATIC,numTris,numVerts);

    if (logLevel > 2) 
      if (numPrints < 5)
      cout << "  writing vertices in 'vec3fa' format" << endl;
    void *posPtr = rtcMapBuffer(model->eScene,eMesh,RTC_VERTEX_BUFFER);
    memcpy(posPtr,posData->data,posData->numBytes);
    rtcUnmapBuffer(model->eScene,eMesh,RTC_VERTEX_BUFFER);

    if (logLevel > 2) 
    if (numPrints < 5)
      cout << "  writing indices in 'vec3i' format" << endl;
    switch (idxData->type) {
    case OSP_vec4i: {
      void *idxPtr = rtcMapBuffer(model->eScene,eMesh,RTC_INDEX_BUFFER);
      for (int i=0;i<idxData->size();i++)  
        ((vec3i*)idxPtr)[i] = (vec3i&)((vec4i*)idxData->data)[i];
    } break;
    case OSP_vec3i: {
      void *idxPtr = rtcMapBuffer(model->eScene,eMesh,RTC_INDEX_BUFFER);
      for (int i=0;i<idxData->size();i++)  
        ((vec3i*)idxPtr)[i] = (vec3i&)((vec3i*)idxData->data)[i];
    } break;
    default:
      Assert2(0,"unsupported triangle array format");
    }
    // memcpy(idxPtr,idxData->data,idxData->numBytes);
    rtcUnmapBuffer(model->eScene,eMesh,RTC_INDEX_BUFFER);

    box3f bounds = embree::empty;
    for (int i=0;i<posData->size();i++)
      bounds.extend(((vec3fa*)posPtr)[i]);

    if (logLevel > 2) 
    if (numPrints < 5) {
      cout << "  created triangle mesh (" << numTris << " tris "
           << ", " << numVerts << " vertices)" << endl;
      cout << "  mesh bounds " << bounds << endl;
    } 
    rtcEnable(model->eScene,eMesh);
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
