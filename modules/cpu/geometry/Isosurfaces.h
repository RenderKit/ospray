// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#include "common/FeatureFlagsEnum.h"
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "Geometry.h"
#include "volume/Volume.h"
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
  Ref<const DataT<float>> isovaluesData;
  Ref<Volume> volume;
  VKLHitIteratorContext vklHitContext = VKLHitIteratorContext();
};

inline FeatureFlags Isosurfaces::getFeatureFlags() const
{
  FeatureFlags ff = Geometry::getFeatureFlags();
  ff |= volume->getFeatureFlags();
  return ff;
}

} // namespace ospray

#endif
