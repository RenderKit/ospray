#include "ospray/common/OSPCommon.h"

namespace ospray {

  struct Prim {
    Prim() {};
    Prim(const box3f &bounds, int64 primID) 
      : bounds(bounds), primID(primID)
    {}
    inline box3f getBounds() const 
    { return bounds; }

    box3f bounds;
    int64 primID;
  };

}
