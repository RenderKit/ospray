/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "Model.ih"

typedef uniform Geometry *uniform uniGeomPtr;
typedef uniform Material *uniform uniMaterialPtr;
typedef uniform Volume   *uniform uniVolumePtr;

export void *uniform Model_create(void *uniform cppE)
{
  uniform Model *uniform model = uniform new uniform Model;
  model->cppEquivalent     = cppE;
  model->embreeSceneHandle = NULL;
  model->geometry          = NULL;
  model->volumes           = NULL;
  return model;
}

export void Model_init(void *uniform _model, uniform int32 numGeometries, uniform int32 numVolumes)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  if (model->embreeSceneHandle)
    print("#osp/model: warning - creating new copy of object without deleting old one..\n");
  model->embreeSceneHandle = rtcNewScene(//RTC_SCENE_STATIC|RTC_SCENE_HIGH_QUALITY,
					 RTC_SCENE_STATIC,//|RTC_SCENE_COMPACT,
                                         //RTC_SCENE_DYNAMIC,
                                         //RTC_SCENE_DYNAMIC|RTC_SCENE_COMPACT,
                                         RTC_INTERSECT_UNIFORM|RTC_INTERSECT_VARYING);

  if (model->geometry) delete[] model->geometry;
  model->geometry = uniform new uniform uniGeomPtr[numGeometries];
  model->geometryCount = numGeometries;

  if (model->volumes) delete[] model->volumes;
  if (numVolumes > 0) model->volumes = uniform new uniform uniVolumePtr[numVolumes];
  model->volumeCount = numVolumes;

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

export void Model_setVolume(void *uniform pointer,
                            uniform int32 index,
                            void *uniform volume)
{
  Model *uniform model = (Model *uniform) pointer;
  model->volumes[index] = (Volume *uniform) volume;
}

