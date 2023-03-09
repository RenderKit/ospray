// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct Empty : public detail::Builder
{
  Empty(bool plane = false)
  {
    addPlane = plane;
  }
  ~Empty() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;

  cpp::World buildWorld() const override;
};

// Inlined definitions ////////////////////////////////////////////////////

void Empty::commit()
{
  Builder::commit();
}

cpp::Group Empty::buildGroup() const
{
  cpp::Group group;
  group.commit();
  return group;
}

cpp::World Empty::buildWorld() const
{
  cpp::World world;

  if (addPlane) {
    std::vector<cpp::Instance> inst;
    inst.push_back(makeGroundPlane(box3f(zero, one)));
    world.setParam("instance", cpp::CopiedData(inst));
  }

  return world;
}

OSP_REGISTER_TESTING_BUILDER(Empty, empty);
OSP_REGISTER_TESTING_BUILDER(Empty(true), nolight);

} // namespace testing
} // namespace ospray
