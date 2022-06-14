// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
// ispc shared
#include "SpheresShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Spheres
    : public AddStructShared<Geometry, ispc::Spheres>
{
  Spheres();
  virtual ~Spheres() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  float radius{.01f}; // default radius, if no per-sphere radius
  Ref<const DataT<vec3f>> vertexData;
  Ref<const DataT<float>> radiusData;
  Ref<const DataT<vec2f>> texcoordData;
};

} // namespace ospray
