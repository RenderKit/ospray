#undef NDEGBUG

// ospray
#include "streamlines.h"
#include "common/data.h"
#include "common/model.h"
// ispc-generated files
#include "streamlines_ispc.h"

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

    Assert(radius > 0.f);
    Assert(vertexData != NULL);
    Assert(indexData != NULL);

    index       = (const uint32*)indexData->data;
    numSegments = indexData->numItems;
    vertex      = (const vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;

    std::cout << "#osp: creating streamlines geometry, "
              << "#verts=" << numVertices << ", "
              << "#segments=" << numSegments << ", "
              << "radius=" << radius << std::endl;
    
    ispc::StreamLineGeometry_set(getIE(),model->getIE(),radius,
                                 (ispc::vec3fa*)vertex,numVertices,
                                 (uint32_t*)index,numSegments);
  }

  OSP_REGISTER_GEOMETRY(StreamLines,streamlines);
}
