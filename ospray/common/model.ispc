#include "model.ih"

typedef uniform Geometry *uniform uniGeomPtr;

export uniform Model *uniform Model_create(void *uniform cppE, uniform int32 numGeometries)
{
  uniform Model *uniform model = uniform new uniform Model;
  model->embreeSceneHandle = rtcNewScene(RTC_SCENE_STATIC,
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
