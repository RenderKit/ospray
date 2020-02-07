// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Instance.h"
#include "Data.h"
// ispc exports
#include "Geometry_ispc.h"
#include "Instance_ispc.h"
#include "OSPCommon_ispc.h"

namespace ospray {

extern "C" void *ospray_getEmbreeDevice();

Instance::Instance(Group *_group)
{
  managedObjectType = OSP_INSTANCE;
  this->ispcEquivalent = ispc::Instance_create(this);

  group = _group;
}

std::string Instance::toString() const
{
  return "ospray::Instance";
}

void Instance::commit()
{
  instanceXfm = getParam<affine3f>("xfm", affine3f(one));
  rcpXfm = rcp(instanceXfm);

  ispc::Instance_set(getIE(),
      group->getIE(),
      (ispc::AffineSpace3f &)instanceXfm,
      (ispc::AffineSpace3f &)rcpXfm);
}

box3f Instance::getBounds() const
{
  return xfmBounds(instanceXfm, group->getBounds());
}

OSPTYPEFOR_DEFINITION(Instance *);

} // namespace ospray
