// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Instance.h"

namespace ospray {

Instance::Instance(Group *_group) : groupAPI(_group)
{
  managedObjectType = OSP_INSTANCE;
}

std::string Instance::toString() const
{
  return "ospray::Instance";
}

void Instance::commit()
{
  group = hasParam("group") ? (Group *)getParamObject("group") : groupAPI;
  if (!group)
    throw std::runtime_error(toString() + " received NULL 'group'");

  motionTransform.readParams(*this);

  // Initialize shared structure
  getSh()->group = group->getSh();
  getSh()->xfm = motionTransform.transform;
  getSh()->rcp_xfm = rcp(getSh()->xfm);
  getSh()->motionBlur = motionTransform.motionBlur;
  getSh()->userID = getParam<uint32>("id", RTC_INVALID_GEOMETRY_ID);
}

box3f Instance::getBounds() const
{
  box3f bounds;
  const box3f groupBounds = group->getBounds();
  // TODO get better bounds:
  // - uses bounds at time 0.5, wrong if different shutter is used and does not
  //   include motion at all
  // alternatives:
  // - merge linear bounds from Embree (which is for time [0, 1]), but this
  //   needs a temporary RTCScene just for this instance
  // - extend over all transformed bounds
  //   - this is too big, because keys outside of time [0, 1] are included
  //   - yet is is also too small, because interpolated transformations can
  //     lead to leaving the bounds of keys
  bounds.extend(xfmBounds(motionTransform.transform, groupBounds));
  return bounds;
}

void Instance::setEmbreeGeom(RTCGeometry geom)
{
  motionTransform.setEmbreeTransform(geom);
  getSh()->geom = geom;
  if (getSh()->motionBlur) {
    rtcGetGeometryTransform(geom,
        .5f,
        RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
        &getSh()->xfm); // for SciVis
    getSh()->rcp_xfm = rcp(getSh()->xfm);
  }
}

OSPTYPEFOR_DEFINITION(Instance *);

} // namespace ospray
