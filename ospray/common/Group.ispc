// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Group.ih"

export void *uniform Group_create(void *uniform cppE)
{
  Group *uniform group = uniform new Group;

  group->cppEquivalent = cppE;

  group->geometricModels = NULL;
  group->numGeometricModels = 0;

  group->volumetricModels = NULL;
  group->numVolumetricModels = 0;

  return (void *uniform)group;
}

export void Group_set(void *uniform _group,
    void *uniform _geomModels,
    uniform int32 numGeomModels,
    void *uniform _volModels,
    uniform int32 numVolModels)
{
  Group *uniform group = (Group * uniform) _group;

  group->geometricModels = (GeometricModel * *uniform) _geomModels;
  group->numGeometricModels = numGeomModels;

  group->volumetricModels = (VolumetricModel * *uniform) _volModels;
  group->numVolumetricModels = numVolModels;
}
