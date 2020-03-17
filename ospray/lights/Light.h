// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/Util.h"

namespace ospray {

//! Base class for Light objects
struct OSPRAY_SDK_INTERFACE Light : public ManagedObject
{
  Light();
  virtual ~Light() override = default;

  static Light *createInstance(const char *type);
  template <typename T>
  static void registerType(const char *type);

  virtual void commit() override;
  virtual std::string toString() const override;

  //! get IE of a second light associated with the light type(if available)
  virtual utility::Optional<void *> getSecondIE();

  vec3f radiance{1.0f, 1.0f, 1.0f};

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<Light> f);
};

OSPTYPEFOR_SPECIALIZATION(Light *, OSP_LIGHT);

// Inlined defintions /////////////////////////////////////////////////////////

template <typename T>
inline void Light::registerType(const char *type)
{
  registerTypeHelper<Light, T>(type);
}

} // namespace ospray
