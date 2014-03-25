#include "streamlines.h"
#include "common/data.h"

namespace ospray {

  void StreamLines::finalize(Model *model) 
  {
    radius = getParam1f("radius",0.f);
    vertexData = getParamData("vertex",NULL);
    indexData = getParamData("index",NULL);

    Assert(radius > 0.f);
    Assert(vertexData != NULL);
    Assert(indexData != NULL);

    index = (const uint32*)indexData->data;
    numSegments = indexData->numItems;
    vertex = (const vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;

    std::cout << "#osp: creating streamlines geometry, "
              << "#verts=" << numVertices << ", "
              << "#segments=" << numSegments << std::endl;
    
    
  }

}
