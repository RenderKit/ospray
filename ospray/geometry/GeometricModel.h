// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "common/Data.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE GeometricModel : public ManagedObject
{
  GeometricModel(Geometry *geometry);
  ~GeometricModel() override = default;

  std::string toString() const override;

  void commit() override;

  Geometry &geometry();

 private:
  Ref<Geometry> geom;
  Ref<const Data> materialData;
  Ref<const DataT<vec4f>> colorData;
  Ref<const DataT<uint8_t>> indexData;
  std::vector<void *> ispcMaterialPtrs;

  friend struct PathTracer; // TODO: fix this!
  friend struct Renderer;
};

OSPTYPEFOR_SPECIALIZATION(GeometricModel *, OSP_GEOMETRIC_MODEL);

// Inlined members //////////////////////////////////////////////////////////

inline Geometry &GeometricModel::geometry()
{
  return *geom;
}

} // namespace ospray
