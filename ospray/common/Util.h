// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "./Managed.h"

namespace ospray {

template <typename OSPRAY_CLASS>
inline OSPRAY_CLASS *createInstanceHelper(
    const std::string &type, FactoryFcn<OSPRAY_CLASS> fcn)
{
  const auto type_string = stringFor(OSPTypeFor<OSPRAY_CLASS *>::value);

  if (fcn) {
    auto *obj = fcn();
    if (obj == nullptr) {
      throw std::runtime_error("Could not find " + type_string
        + " of type: " + type
        + ".  Make sure you have the correct OSPRay libraries "
        "linked and initialized.");
    }
    return obj;
  } else {
    postStatusMsg(OSP_LOG_WARNING) << "  WARNING: unrecognized " << type_string
                                   << " type '" << type << "'.";
    return nullptr;
  }
}

template <typename BASE_CLASS, typename CHILD_CLASS>
inline void registerTypeHelper(const char *type)
{
  BASE_CLASS *(*fcn)() = &allocate_object<BASE_CLASS, CHILD_CLASS>;
  BASE_CLASS::registerType(type, fcn);
}

} // namespace ospray
