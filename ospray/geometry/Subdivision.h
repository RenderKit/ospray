// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "common/Data.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Subdivision : public Geometry
{
  Subdivision();
  virtual ~Subdivision() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  float level{0.f};

  Ref<const DataT<vec3f>> vertexData;
  Ref<const DataT<vec4f>> colorsData;
  Ref<const DataT<vec2f>> texcoordData;
  Ref<const DataT<uint32_t>> indexData;
  Ref<const DataT<float>> indexLevelData;
  Ref<const DataT<uint32_t>> facesData;
  Ref<const DataT<vec2ui>> edge_crease_indicesData;
  Ref<const DataT<float>> edge_crease_weightsData;
  Ref<const DataT<uint32_t>> vertex_crease_indicesData;
  Ref<const DataT<float>> vertex_crease_weightsData;

  OSPSubdivisionMode mode;
};

} // namespace ospray
