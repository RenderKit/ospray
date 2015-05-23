#include "ospray/common/OSPCommon.h"

namespace ospray {

  struct Triangle {
    Triangle() {};
    Triangle(int geomID, int primID, vec3f a, vec3f b, vec3f c) 
      : geomID(geomID), primID(primID), a(a), b(b), c(c)
    {}

    vec3f a,b,c;
    int geomID,primID;

    box3f getBounds() const { 
      box3f bb = embree::empty;
      bb.extend(a);
      bb.extend(b);
      bb.extend(c);
      return bb;
    }
  };

}
