#include "Block.h"

namespace ospray {
  
  struct TriangleStream : public embree::RefCount {
    TriangleStream() 
      : bounds(embree::empty), numTriangles(0)
    {}

    void flush();
    void push(const Triangle &tri);
    
    std::vector<Ref<BlockRef> > blockRefs;
    Block buffer;
    box3f bounds;
    size_t numTriangles;
  };

}
