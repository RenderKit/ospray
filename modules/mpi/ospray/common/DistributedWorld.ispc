// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.ih"
#include "common/World.ih"

export void *uniform DistributedWorld_create()
{
  DistributedWorld *uniform world = uniform new DistributedWorld;
  World_Constructor(&world->super);
  return (void *uniform)world;
}

export void DistributedWorld_set(void *uniform _world,
    void *uniform _regions,
    uniform int numRegions,
    void *uniform _regionScene)
{
  DistributedWorld *uniform world = (DistributedWorld * uniform) _world;
  world->regions = (box3f * uniform) _regions;
  world->numRegions = numRegions;
  world->regionScene = (RTCScene)_regionScene;
}
