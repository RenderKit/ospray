#include "testMC.h"
#include <ospray/ospray.h>

#include "testMC_ispc.h"

namespace ospray {
  float sqr(float f) { return f*f; }

  Field *createTestField(const vec3i &size)
  {
    Field *field = new Field(size);
    for (int z=0;z<size.z;z++)
      for (int y=0;y<size.y;y++)
        for (int x=0;x<size.x;x++) {
          float fx = x / float(size.x);
          float fy = y / float(size.y);
          float fz = z / float(size.z);

          // float f = cosf(1*x+1.3*y*z)*sinf(1.11*z+1.7*x+2.3*y);
          // float f = cosf(7*x+3*y*z)*sinf(11*z+7*x+3*y);
          float f = sqrtf(sqr(fx-.35f)+sqr(fy-.55)+sqr(fz-.59f)); // + .3f*drand48();
          
          field->set(vec3i(x,y,z),f);
        }
    return field;
  }

  void testMC(int ac, const char **av)
  {
    ospInit(&ac,av);
    Field *field = createTestField(vec3i(256));
    field->ie = ispc::createField(field->size.x,field->size.y,field->size.z,field->voxel);
    ispc::marchingCubes(field->ie,0.4f);
  }
}

int main(int ac, const char **av)
{
  ospray::testMC(ac,av);
}


