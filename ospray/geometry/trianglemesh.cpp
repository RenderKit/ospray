#include "trianglemesh.h"
#include "../common/model.h"
// embree stuff
#include "rtcore.h"
#include "rtcore_scene.h"
#include "rtcore_geometry.h"

#define RTC_INVALID_ID INVALID_GEOMETRY_ID

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
    if (numPrints == 5)
      cout << "(all future printouts for triangle mesh creation will be emitted)" << endl;
    
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

    if (numPrints < 5)
    cout << "  writing vertices in 'vec3fa' format" << endl;
    void *posPtr = rtcMapBuffer(model->eScene,eMesh,RTC_VERTEX_BUFFER);
    memcpy(posPtr,posData->data,posData->numBytes);
    rtcUnmapBuffer(model->eScene,eMesh,RTC_VERTEX_BUFFER);

    if (numPrints < 5)
    cout << "  writing indices in 'vec3i' format" << endl;
    void *idxPtr = rtcMapBuffer(model->eScene,eMesh,RTC_INDEX_BUFFER);
    for (int i=0;i<idxData->size();i++)  {
      ((vec3i*)idxPtr)[i] = (vec3i&)((vec4i*)idxData->data)[i];
    }
    // memcpy(idxPtr,idxData->data,idxData->numBytes);
    rtcUnmapBuffer(model->eScene,eMesh,RTC_INDEX_BUFFER);

    box3f bounds = embree::empty;
    for (int i=0;i<posData->size();i++)
      bounds.extend(((vec3fa*)posPtr)[i]);

    if (numPrints < 5) {
      cout << "  created triangle mesh (" << numTris << " tris "
           << ", " << numVerts << " vertices)" << endl;
      cout << "  mesh bounds " << bounds << endl;
    } 
    rtcEnable(model->eScene,eMesh);
  }
}
