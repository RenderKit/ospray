// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/Util.h"
// ispc shared
#include "TextureShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Texture
    : AddStructShared<ManagedObject, ispc::Texture>
{
  Texture();
  virtual ~Texture() override = default;

  virtual std::string toString() const override;

  static Texture *createInstance(const char *type);
  template <typename T>
  static void registerType(const char *type);

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<Texture> f);
};

OSPTYPEFOR_SPECIALIZATION(Texture *, OSP_TEXTURE);

// Inlined definitions /////////////////////////////////////////////////////////

template <typename T>
inline void Texture::registerType(const char *type)
{
  registerTypeHelper<Texture, T>(type);
}

} // namespace ospray
