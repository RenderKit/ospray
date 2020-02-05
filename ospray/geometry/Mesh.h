// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "common/Data.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Mesh : public Geometry
{
  Mesh();
  virtual ~Mesh() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  Ref<const DataT<vec3f>> vertexData;
  Ref<const DataT<vec3f>> normalData;
  Ref<const Data> colorData;
  Ref<const DataT<vec2f>> texcoordData;
  Ref<const Data> indexData;
};

} // namespace ospray
