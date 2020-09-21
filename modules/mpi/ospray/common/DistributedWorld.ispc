// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.ih"
#include "common/World.ih"

export void *uniform DistributedWorld_create(void *uniform cppE)
{
  DistributedWorld *uniform world = uniform new DistributedWorld;

  // TODO: this should be done in the World_Constructor and called here
  world->super.cppEquivalent = cppE;
  world->super.geometriesInst = NULL;
  world->super.numGeometriesInst = 0;
  world->super.volumesInst = NULL;
  world->super.numVolumesInst = 0;
  world->super.clippersInst = NULL;
  world->super.numClippersInst = 0;
               
  world->super.embreeSceneHandleGeometries = NULL;
  world->super.embreeSceneHandleVolumes = NULL;
  world->super.embreeSceneHandleClippers = NULL;
               
  world->super.numInvertedClippers = 0;
  world->super.scivisData.lights = NULL;

  return (void *uniform)world;
}

export void DistributedWorld_set(
    void *uniform _world, void *uniform _regions, uniform int numRegions)
{
  DistributedWorld *uniform world = (DistributedWorld * uniform) _world;
  world->regions = (uniform Region * uniform) _regions;
  world->numRegions = numRegions;
}
