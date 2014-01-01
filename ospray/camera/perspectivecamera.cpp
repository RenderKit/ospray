#include "perspectivecamera.h"
#include <limits>
// ispc-side stuff
#include "perspectivecamera_ispc.h"

namespace ospray {

  PerspectiveCamera::PerspectiveCamera()
  {
    ispc::PerspectiveCamera_create(this,ispcEquivalent,(void*&)ispcData);
  }
  void PerspectiveCamera::commit()
  {
    // ------------------------------------------------------------------
    // first, "parse" the expected parameters
    // ------------------------------------------------------------------
    pos    = getParam3f("pos",vec3fa(0.f));
    dir    = getParam3f("dir",vec3fa(0.f,0.0f,1.f));
    up     = getParam3f("up", vec3fa(0.f,1.0f,0.f));
    near   = getParamf("near",0.f);
    far    = getParamf("far", std::numeric_limits<float>::infinity());
    fovy   = getParamf("fovy",60.f);
    aspect = getParamf("aspect",1.f);

    PING;
    PRINT(pos);
    PRINT(dir);
    PRINT(up);

    // ------------------------------------------------------------------
    // now, update the local precomptued values
    // ------------------------------------------------------------------
    vec3f dz = normalize(dir);
    vec3f dx = normalize(cross(dz,up));
    vec3f dy = normalize(cross(dx,dz));
    PRINT(dz);
    PRINT(dx);
    PRINT(dy);

    float imgPlane_size_x = 2.f*sinf(fovy/2.f*M_PI/180.);
    float imgPlane_size_y = imgPlane_size_x * aspect;
    dir_00
      = dz
      - (.5f * imgPlane_size_x) * dx
      - (.5f * imgPlane_size_y) * dy;
    dir_du = dx;
    dir_dv = dy;

    (vec3f&)ispcData->org    = pos;
    (vec3f&)ispcData->dir_00 = dir_00;
    (vec3f&)ispcData->dir_du = dir_du;
    (vec3f&)ispcData->dir_dv = dir_dv;
  }

  OSP_REGISTER_CAMERA(PerspectiveCamera,perspective);

}
