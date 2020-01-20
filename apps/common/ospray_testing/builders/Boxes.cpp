// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon::math;

namespace ospray {
namespace testing {

struct Boxes : public detail::Builder
{
  Boxes() = default;
  ~Boxes() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;

 private:
  vec3i dimensions{4};
};

// Inlined definitions ////////////////////////////////////////////////////

void Boxes::commit()
{
  Builder::commit();

  dimensions = getParam<vec3i>("dimensions", vec3i(4));

  addPlane = false;
}

cpp::Group Boxes::buildGroup() const
{
  cpp::Geometry boxGeometry("box");

  index_sequence_3D numBoxes(dimensions);

  std::vector<box3f> boxes;
  std::vector<vec4f> color;

  auto dim = reduce_max(dimensions);

  for (auto i : numBoxes) {
    auto i_f = static_cast<vec3f>(i);

    auto lower = i_f * 5.f;
    auto upper = lower + (0.75f * 5.f);
    boxes.emplace_back(lower, upper);

    auto box_color = (0.8f * i_f / dim) + 0.2f;
    color.emplace_back(box_color.x, box_color.y, box_color.z, 1.f);
  }

  boxGeometry.setParam("box", cpp::Data(boxes));
  boxGeometry.commit();

  cpp::GeometricModel model(boxGeometry);

  model.setParam("color", cpp::Data(color));

  if (rendererType == "pathtracer" || rendererType == "scivis") {
    cpp::Material material(rendererType, "obj");
    material.commit();
    model.setParam("material", material);
  }

  model.commit();

  cpp::Group group;

  group.setParam("geometry", cpp::Data(model));
  group.commit();

  return group;
}

OSP_REGISTER_TESTING_BUILDER(Boxes, boxes);

} // namespace testing
} // namespace ospray
