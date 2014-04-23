#undef NDEGBUG

// ospray
#include "instance.h"
#include "common/model.h"
// ispc-generated files
#include "instance_ispc.h"

namespace ospray {

  Instance::Instance()
  {
    this->ispcEquivalent = ispc::InstanceGeometry_create(this);
  }

  void Instance::finalize(Model *model) 
  {
    xfm.l.vx = getParam3f("xfm.l.vx",vec3f(1.f,0.f,0.f));
    xfm.l.vy = getParam3f("xfm.l.vy",vec3f(0.f,1.f,0.f));
    xfm.l.vz = getParam3f("xfm.l.vz",vec3f(0.f,0.f,1.f));
    xfm.p   = getParam3f("xfm.p",vec3f(0.f,0.f,0.f));
    instancedScene = (Model *)getParamObject("model",NULL);
    
    embreeGeomID = rtcNewInstance(model->embreeSceneHandle,
                                  instancedScene->embreeSceneHandle);
    rtcSetTransform(model->embreeSceneHandle,embreeGeomID,
                    RTC_MATRIX_COLUMN_MAJOR_ALIGNED16,
                    (const float *)&xfm);
    rtcEnable(model->embreeSceneHandle,embreeGeomID);
    // ispc::InstanceGeometry_set(getIE(),model->getIE(),radius,
    //                              (ispc::vec3fa*)vertex,numVertices,
    //                              (uint32_t*)index,numSegments);
  }

  OSP_REGISTER_GEOMETRY(Instance,instance);
}
