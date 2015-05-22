#include "TriangleStream.h"

namespace ospray {

  void TriangleStream::flush()
  {
    if (numTriangles == 0) return;

    size_t myID = (size_t)pthread_self();
    // printf("ID %li DOING flush\n",myID);

    Ref<BlockRef> newBlock = new BlockRef;
    // printf("ID %li saving\n",myID);
    newBlock->saveBlock(buffer);
    // printf("ID %li pushing\n",myID);
    blockRefs.push_back(newBlock);
    // printf("ID %li bla\n",myID);

    buffer.clear();
  }
  
  void TriangleStream::push(const Triangle &tri)
  {
    bounds.extend(tri.a);
    bounds.extend(tri.b);
    bounds.extend(tri.c);
    numTriangles++;
    if (buffer.push(tri)) return;

    flush();
    buffer.push(tri);
  }
  
}
