// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedWorld.ih"
#include "common/World.ih"

export void *uniform DistributedWorld_create(void *uniform cppE)
{
  DistributedWorld *uniform world = uniform new DistributedWorld;
  World_Constructor(&world->super, cppE);
  return (void *uniform)world;
}

export void DistributedWorld_set(
    void *uniform _world, void *uniform _regions, uniform int numRegions)
{
  DistributedWorld *uniform world = (DistributedWorld * uniform) _world;
  world->regions = (uniform Region * uniform) _regions;
  world->numRegions = numRegions;
}
