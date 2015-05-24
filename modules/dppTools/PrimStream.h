#include "Block.h"

namespace ospray {
  
  struct PrimStream : public embree::RefCount {
    PrimStream() 
      : bounds(embree::empty), numPrims(0)
    {}
    virtual ~PrimStream() {
      // BlockRef *br = blockRefs[0].ptr;
      blockRefs.clear();
    }

    void flush();
    void push(const Prim &tri);
    
    std::vector<Ref<BlockRef> > blockRefs;
    Block buffer;
    box3f bounds;
    size_t numPrims;
  };

}
