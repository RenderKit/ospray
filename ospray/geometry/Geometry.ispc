// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.ih"

void Geometry_Constructor(
    Geometry *uniform self, uniform Geometry_postIntersectFct postIntersect)
{
  self->numPrimitives = 0;
  self->postIntersect = postIntersect;
  self->sampleArea = NULL;
  self->getAreas = NULL;
}
