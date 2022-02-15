// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Spheres : public Geometry
{
  Spheres();
  virtual ~Spheres() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  float radius{0.01}; // default radius, if no per-sphere radius
  Ref<const DataT<vec3f>> vertexData;
  Ref<const DataT<float>> radiusData;
  Ref<const DataT<vec2f>> texcoordData;
};

} // namespace ospray
