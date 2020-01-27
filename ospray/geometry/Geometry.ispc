// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.ih"

static void Geometry_Constructor(Geometry *uniform self,
    void *uniform cppEquivalent,
    uniform Geometry_postIntersectFct postIntersect)
{
  self->cppEquivalent = cppEquivalent;
  self->numPrimitives = 0;
  self->postIntersect = postIntersect;
  self->sampleArea = NULL;
  self->getAreas = NULL;
}

export void Geometry_set_geomID(void *uniform _self, uniform int32 geomID)
{
  Geometry *uniform self = (Geometry *uniform)_self;
  self->geomID = geomID;
}
