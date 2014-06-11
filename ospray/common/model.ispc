#include "model.ih"

typedef uniform Geometry *uniform uniGeomPtr;

// ISPC bug: can't export pointer to model because model contains (through N indirections) a function pointer that has a varying parameter ...
//export uniform Model *uniform
export void *uniform Model_create(void *uniform cppE, uniform int32 numGeometries)
{
  uniform Model *uniform model = uniform new uniform Model;
  model->embreeSceneHandle = rtcNewScene(//RTC_SCENE_STATIC|RTC_SCENE_HIGH_QUALITY,
                                         //RTC_SCENE_STATIC|RTC_SCENE_COMPACT,
                                         RTC_SCENE_DYNAMIC,
                                         //RTC_SCENE_DYNAMIC|RTC_SCENE_COMPACT,
                                         RTC_INTERSECT_UNIFORM|RTC_INTERSECT_VARYING);
  model->cppEquivalent     = cppE;
  model->geometry = uniform new uniform uniGeomPtr[numGeometries];
  
  return model;
}

export uniform RTCScene Model_getEmbreeSceneHandle(void *uniform _model)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  return model->embreeSceneHandle;
}

export void Model_setGeometry(void *uniform _model,
                              uniform int32 geomID,
                              void *uniform _geom)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Geometry *uniform geom = (uniform Geometry *uniform)_geom;
  model->geometry[geomID] = geom;
}

// void postIntersect(uniform Model *uniform model,
//                    varying Ray &ray,
//                    uniform int64 flags)
// {
//   print("pi\n");
// }
