// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Managed.h"

namespace ospray {

ManagedObject::~ManagedObject()
{
  try {
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;
      if (param.data.valid() && param.data.is<OSP_PTR>()) {
        auto *obj = param.data.get<OSP_PTR>();
        if (obj != nullptr)
          obj->refDec();
      }
    });
  } catch (const std::exception &e) {
    ospray::handleError(OSP_UNKNOWN_ERROR, e.what());
  }
}

void ManagedObject::commit() {}

std::string ManagedObject::toString() const
{
  return "ospray::ManagedObject";
}

void ManagedObject::checkUnused()
{
  for (auto p = params_begin(); p != params_end(); ++p) {
    if (!(*p)->query) {
      postStatusMsg(OSP_LOG_WARNING)
          << toString() << ": found unused (or of wrong data type) parameter '"
          << (*p)->name << "'";
    }
  }
}

box3f ManagedObject::getBounds() const
{
  return box3f(empty);
}

OSPTYPEFOR_DEFINITION(ManagedObject *);

} // namespace ospray
