// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.ih"
#include "common/World.ih"

export void *uniform DistributedWorld_create(void *uniform cppE)
{
  uniform DistributedWorld *uniform world =
      uniform new uniform DistributedWorld;
  world->super.cppEquivalent = cppE;
  world->super.instances = NULL;

  world->super.embreeSceneHandleGeometries = NULL;
  world->super.embreeSceneHandleVolumes = NULL;
  return (void *uniform)world;
}

export void DistributedWorld_set(
    void *uniform _world, void *uniform _regions, uniform int numRegions)
{
  uniform DistributedWorld *uniform world =
      (uniform DistributedWorld * uniform) _world;
  world->regions = (uniform Region * uniform) _regions;
  world->numRegions = numRegions;
}
