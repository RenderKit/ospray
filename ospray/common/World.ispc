// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "World.ih"

export void *uniform World_create(void *uniform cppE)
{
  World *uniform world = uniform new World;

  world->cppEquivalent = cppE;
  world->instances = NULL;

  world->embreeSceneHandleGeometries = NULL;
  world->embreeSceneHandleVolumes = NULL;

  return (void *uniform)world;
}

export void World_set(void *uniform _self,
    void *uniform _instances,
    uniform int32 numInstances,
    void *uniform embreeGeometryScene,
    void *uniform embreeVolumeScene)
{
  World *uniform self = (World * uniform) _self;
  Instance **uniform instances = (Instance * *uniform) _instances;

  self->instances = instances;
  self->numInstances = numInstances;

  self->embreeSceneHandleGeometries = (RTCScene)embreeGeometryScene;
  self->embreeSceneHandleVolumes = (RTCScene)embreeVolumeScene;
}
