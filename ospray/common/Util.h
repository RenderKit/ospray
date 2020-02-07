// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../api/objectFactory.h"
#include "./Managed.h"

#include <map>

namespace ospray {

template <typename OSPRAY_CLASS, OSPDataType OSP_TYPE>
inline OSPRAY_CLASS *createInstanceHelper(const std::string &type)
{
  static_assert(std::is_base_of<ManagedObject, OSPRAY_CLASS>::value,
      "createInstanceHelper<>() is only for OSPRay classes, not"
      " generic types!");

  auto *object = objectFactory<OSPRAY_CLASS, OSP_TYPE>(type);

  // Denote the subclass type in the ManagedObject base class.
  if (object) {
    object->managedObjectType = OSP_TYPE;
  }

  return object;
}

} // namespace ospray
