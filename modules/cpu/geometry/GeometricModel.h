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

struct Material;

struct OSPRAY_SDK_INTERFACE GeometricModel
    : public AddStructShared<ISPCDeviceObject, ispc::GeometricModel>
{
  GeometricModel(api::ISPCDevice &device, Geometry *geometry);
  ~GeometricModel() override = default;

  std::string toString() const override;

  void commit() override;

  Geometry &geometry();
  RTCGeometry embreeGeometryHandle() const;

  bool invertedNormals() const;

  bool hasEmissiveMaterials(
      Ref<const DataT<Material *>> rendererMaterials) const;

  FeatureFlags getFeatureFlags() const;

 private:
  Ref<Geometry> geom;
  const Ref<Geometry> geomAPI;
  Ref<const Data> materialData;
  Ref<const DataT<vec4f>> colorData;
  Ref<const DataT<uint8_t>> indexData;
  std::unique_ptr<BufferShared<ispc::Material *>> materialArray;
  std::unique_ptr<BufferShared<uint32_t>> materialIDArray;

  FeatureFlagsOther featureFlagsOther{FFO_NONE};
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

inline FeatureFlags GeometricModel::getFeatureFlags() const
{
  FeatureFlags ff = geom->getFeatureFlags();
  ff.other |= featureFlagsOther;
  return ff;
}

} // namespace ospray
