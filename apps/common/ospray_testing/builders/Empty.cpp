// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"

using namespace ospcommon::math;

namespace ospray {
namespace testing {

struct Empty : public detail::Builder
{
  Empty() = default;
  ~Empty() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;
};

// Inlined definitions ////////////////////////////////////////////////////

void Empty::commit()
{
  Builder::commit();
  addPlane = false;
}

cpp::Group Empty::buildGroup() const
{
  cpp::Group group;
  group.commit();
  return group;
}

OSP_REGISTER_TESTING_BUILDER(Empty, empty);

} // namespace testing
} // namespace ospray
