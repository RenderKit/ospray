#include "model.ih"

export uniform Model *uniform 
ispc_createModel(void *uniform cppE, uniform int32 numGeometries, 
                 uniform RTCScene *uniform embreeHandle)
{
  *embreeHandle = rtcNewScene(RTC_SCENE_STATIC,
                              RTC_INTERSECT_UNIFORM|RTC_INTERSECT_VARYING);
  uniform Model *uniform model = uniform new uniform Model;
  model->embreeSceneHandle = *embreeHandle;
  model->cppEquivalent     = cppE;
  
  return model;
}
