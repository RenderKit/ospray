#undef NDEGBUG

// ospray
#include "streamlines.h"
#include "common/data.h"
#include "common/model.h"
// ispc-generated files
#include "streamlines_ispc.h"

namespace ospray {

  void StreamLines::finalize(Model *model) 
  {
    PING;
    radius = getParam1f("radius",0.01f);
    vertexData = getParamData("vertex",NULL);
    indexData = getParamData("index",NULL);

    Assert(radius > 0.f);
    Assert(vertexData != NULL);
    Assert(indexData != NULL);

    index       = (const uint32*)indexData->data;
    numSegments = indexData->numItems;
    vertex      = (const vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;

    std::cout << "#osp: creating streamlines geometry, "
              << "#verts=" << numVertices << ", "
              << "#segments=" << numSegments << std::endl;
    
    PRINT(radius);
    ispc::StreamLineGeometry_create((ispc::__RTCScene*)model->embreeSceneHandle,
                                    this,radius,
                                    (ispc::vec3fa*)vertex,numVertices,
                                    (uint32_t*)index,numSegments);
  }

  OSP_REGISTER_GEOMETRY(StreamLines,streamlines);
}
