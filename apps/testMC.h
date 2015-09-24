#include <ospray/Common/OSPCommon.h>

namespace ospray {

  struct Field {
    vec3i size;
    float *voxel;
    void *ie;

    Field(const vec3i &size) : size(size), ie(NULL) { voxel = new float[((long)size.x)*size.y*size.z]; }
    inline long index(const vec3i &coord) const {
      return ((((long)coord.z)*size.y + coord.y)*size.x + coord.x);
    }
    inline void set(const vec3i &coord, float val)
    { voxel[index(coord)] = val; }
  };
}

