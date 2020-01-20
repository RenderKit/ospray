// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "box.ih"

void print_box(const uniform box3f &bbox)
{
  print("[(%f %f %f)-(%f %f %f)]",
      bbox.lower.x,
      bbox.lower.y,
      bbox.lower.z,
      bbox.upper.x,
      bbox.upper.y,
      bbox.upper.z);
}

void print_box(const uniform box3fa &bbox)
{
  print("[(%f %f %f)-(%f %f %f)]",
      bbox.lower.x,
      bbox.lower.y,
      bbox.lower.z,
      bbox.upper.x,
      bbox.upper.y,
      bbox.upper.z);
}
