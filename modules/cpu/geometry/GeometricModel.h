// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "common/Data.h"
// ispc shared
#include "GeometricModelShared.h"

namespace ispc {
struct Material;
}

namespace ospray {

struct OSPRAY_SDK_INTERFACE GeometricModel
    : public AddStructShared<ManagedObject, ispc::GeometricModel>
{
  GeometricModel(Geometry *geometry);
  ~GeometricModel() override = default;

  std::string toString() const override;

  void commit() override;

  Geometry &geometry();
  RTCGeometry embreeGeometryHandle() const;

  bool invertedNormals() const;

 private:
  Ref<Geometry> geom;
  const Ref<Geometry> geomAPI;
  Ref<const Data> materialData;
  Ref<const DataT<vec4f>> colorData;
  Ref<const DataT<uint8_t>> indexData;
  std::vector<ispc::Material *> ispcMaterialPtrs;

  friend struct PathTracer; // TODO: fix this!
  friend struct Renderer;
};

OSPTYPEFOR_SPECIALIZATION(GeometricModel *, OSP_GEOMETRIC_MODEL);

// Inlined members //////////////////////////////////////////////////////////

inline Geometry &GeometricModel::geometry()
{
  return *geom;
}

inline RTCGeometry GeometricModel::embreeGeometryHandle() const
{
  return geom->getEmbreeGeometry();
}

inline bool GeometricModel::invertedNormals() const
{
  return getSh()->invertedNormals;
}

} // namespace ospray
