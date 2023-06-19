// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#include "common/FeatureFlagsEnum.h"
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "Geometry.h"
#include "volume/VolumetricModel.h"
// ispc shared
#include "IsosurfacesShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Isosurfaces
    : public AddStructShared<Geometry, ispc::Isosurfaces>
{
  Isosurfaces(api::ISPCDevice &device);
  virtual ~Isosurfaces() override;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

  FeatureFlags getFeatureFlags() const override;

 protected:
  // Data members //

  Ref<const DataT<float>> isovaluesData;
  // For backwards compatibility, a volumetric model was used to set
  // the volume and color
  Ref<VolumetricModel> model;
  Ref<Volume> volume;
  VKLHitIteratorContext vklHitContext = VKLHitIteratorContext();
};

inline FeatureFlags Isosurfaces::getFeatureFlags() const
{
  FeatureFlags ff = Geometry::getFeatureFlags();
  if (model) {
    ff |= model->getFeatureFlags();
  } else {
    ff |= volume->getFeatureFlags();
  }
  return ff;
}

} // namespace ospray

#endif
