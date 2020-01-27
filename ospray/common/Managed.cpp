// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Managed.h"
#include "Data.h"
#include "OSPCommon_ispc.h"

namespace ospray {

ManagedObject::~ManagedObject()
{
  ispc::delete_uniform(ispcEquivalent);
  ispcEquivalent = nullptr;

  std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
    auto &param = *p;
    if (param.data.is<OSP_PTR>()) {
      auto *obj = param.data.get<OSP_PTR>();
      if (obj != nullptr)
        obj->refDec();
    }
  });
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

ManagedObject *ManagedObject::getParamObject(
    const char *name, ManagedObject *valIfNotFound)
{
  return getParam<ManagedObject *>(name, valIfNotFound);
}

OSPTYPEFOR_DEFINITION(ManagedObject *);

} // namespace ospray
