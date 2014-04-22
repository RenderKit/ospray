#include "trianglemesh.h"
#include "../common/model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
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
    RTCScene embreeSceneHandle = model->embreeSceneHandle;

    Param *posParam = findParam("position");
    Assert2(posParam != NULL, "triangle mesh geometry does not have a 'position' array");
    posData = dynamic_cast<Data*>(posParam->ptr);
    Assert2(posData != NULL,"invalid position array");
    Assert2(posData->type == OSP_vec3fa, "invalid triangle mesh vertex format");

    Param *idxParam = findParam("index");
    Assert2(idxParam != NULL, "triangle mesh geometry does not have a 'index' array");
    idxData = dynamic_cast<Data*>(idxParam->ptr);
    Assert2(idxData != NULL, "invalid index array");
    Assert2(idxData->type == OSP_vec3i, "invalid triangle index format");

    size_t numTris  = idxData->size();
    size_t numVerts = posData->size();

    eMesh = rtcNewTriangleMesh(embreeSceneHandle,RTC_GEOMETRY_STATIC,numTris,numVerts);

    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_VERTEX_BUFFER,
                 posData->data,0,ospray::sizeOf(posData->type));
    rtcSetBuffer(embreeSceneHandle,eMesh,RTC_INDEX_BUFFER,
                 idxData->data,0,ospray::sizeOf(idxData->type));

    box3f bounds = embree::empty;
    
    if (posData->type == OSP_vec3fa) {
      for (int i=0;i<posData->size();i++) 
        bounds.extend(((vec3fa*)posData->data)[i]);
    } else {
      for (int i=0;i<posData->size();i++)
        bounds.extend(((vec3f*)posData->data)[i]);
    }

    if (logLevel > 2) 
      if (numPrints < 5) {
        cout << "  created triangle mesh (" << numTris << " tris "
             << ", " << numVerts << " vertices)" << endl;
        cout << "  mesh bounds " << bounds << endl;
      } 
    rtcEnable(model->embreeSceneHandle,eMesh);

    // if (getIE())
    //   ispc::TriangleMesh_destroy(getIE());
    // ispcEquivalent = ispc::TriangleMesh_create(
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
