// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Instance.h"
#include "Data.h"
// ispc exports
#include "common/Instance_ispc.h"
#include "common/OSPCommon_ispc.h"
#include "geometry/Geometry_ispc.h"

namespace ospray {

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
  MotionTransform::commit();

  ispc::Instance_set(getIE(),
      group->getIE(),
      (ispc::AffineSpace3f &)(*motionTransforms)[0],
      motionBlur);
}

box3f Instance::getBounds() const
{
  box3f bounds;
  const box3f groupBounds = group->getBounds();
  // TODO get better bounds:
  // - this is too big, because keys outside of time [0, 1] are included
  // - yet is is also too small, because interpolated transformations can lead
  //   to leaving the bounds of keys
  // alternatives:
  // - use bounds at time 0.5, wrong if different shutter is used and does not
  //   include motion at all
  // - merge linear bounds from Embree (which is for time [0, 1]), but this
  //   needs a temporary RTCScene just for this instance
  for (auto &&xfm : *motionTransforms)
    bounds.extend(xfmBounds(xfm, groupBounds));
  return bounds;
}

OSPTYPEFOR_DEFINITION(Instance *);

} // namespace ospray
