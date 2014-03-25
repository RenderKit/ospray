#include "streamlines.h"

namespace ospray {

  void StreamLines::finalize(Model *model) 
  {
    radius = get1f("radius",0.f);
    vertexData = getParam("vertex",NULL);

    Assert(radius > 0.f);
    Assert(vertexData != NULL);
    Assert(indexData != NULL);

    index = (const uint32*)indexData->ptr;
    numSegments = indexData->numItems;
    vertex = (const vec3fa*)vertexData->ptr;
    numVertices = vertexData->numItems;

    std::cout << "#osp: creating streamlines geometry, "
              << "#verts=" << numVertices << ", "
              << "#segments=" << numSegments << std::endl;
    
    
  }

}
