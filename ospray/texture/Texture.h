// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Texture : public ManagedObject
{
  Texture();
  virtual ~Texture() override = default;

  virtual std::string toString() const override;

  static Texture *createInstance(const char *type);
};

OSPTYPEFOR_SPECIALIZATION(Texture *, OSP_TEXTURE);

/*! \brief registers a internal ospray::<ClassName> geometry under
    the externally accessible name "external_name"

    \internal This currently works by defining a extern "C" function
    with a given predefined name that creates a new instance of this
    geometry. By having this symbol in the shared lib ospray can
    lateron always get a handle to this fct and create an instance
    of this geometry.
*/
#define OSP_REGISTER_TEXTURE(InternalClass, external_name)                     \
  OSP_REGISTER_OBJECT(::ospray::Texture, texture, InternalClass, external_name)

} // namespace ospray
