// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDeviceObject.h"
#include "common/FeatureFlagsEnum.h"
#include "common/ObjectFactory.h"
#include "ispcrt.h"

namespace ispc {
struct Light;
struct Instance;
} // namespace ispc

namespace ospray {

//! Base class for Light objects
struct OSPRAY_SDK_INTERFACE Light
    : public ISPCDeviceObject,
      public ObjectFactory<Light, api::ISPCDevice &>
{
  Light(api::ISPCDevice &device, const FeatureFlagsOther ffo);
  virtual ~Light() override = default;

  virtual uint32_t getShCount() const;
  virtual ISPCRTMemoryView createSh(
      uint32_t index, const ispc::Instance *instance = nullptr) const = 0;
  virtual void commit() override;
  virtual std::string toString() const override;

  bool visible{true};
  vec3f coloredIntensity{1.0f, 1.0f, 1.0f};
  OSPIntensityQuantity intensityQuantity = OSP_INTENSITY_QUANTITY_UNKNOWN;

  FeatureFlags getFeatureFlags() const;

 protected:
  FeatureFlagsOther featureFlags;

  void queryIntensityQuantityType(const OSPIntensityQuantity &defaultIQ);
};

OSPTYPEFOR_SPECIALIZATION(Light *, OSP_LIGHT);

// Inlined definitions /////////////////////////////////////////////////////////

inline uint32_t Light::getShCount() const
{
  return 1;
}

inline FeatureFlags Light::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.other = featureFlags;
  return ff;
}

} // namespace ospray
