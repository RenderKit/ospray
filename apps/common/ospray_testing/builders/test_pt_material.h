// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct PtMaterial : public detail::Builder
{
  PtMaterial() = default;
  ~PtMaterial() override = default;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;

  virtual std::vector<cpp::Material> buildMaterials() const = 0;
};

// Inlined definitions ////////////////////////////////////////////////////

inline cpp::Group PtMaterial::buildGroup() const
{
  cpp::Geometry sphereGeometry("sphere");

  std::vector<cpp::Material> materials = buildMaterials();
  const int dimCount = sqrtf(materials.size());
  const int dimSize = dimCount - 1;

  index_sequence_2D numSpheres(dimCount);

  std::vector<vec3f> spheres;
  std::vector<uint8_t> index;
  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    spheres.emplace_back(dimSize / 2.f - i_f.x, dimSize / 2.f - i_f.y, .6f);
    index.push_back(numSpheres.flatten(i));
  }

  sphereGeometry.setParam("sphere.position", cpp::CopiedData(spheres));
  sphereGeometry.setParam("radius", 0.4f);
  sphereGeometry.commit();

  cpp::GeometricModel model(sphereGeometry);

  model.setParam("material", cpp::CopiedData(materials));
  model.setParam("index", cpp::CopiedData(index));
  model.commit();

  cpp::Group group;

  group.setParam("geometry", cpp::CopiedData(model));
  group.commit();

  return group;
}

inline cpp::World PtMaterial::buildWorld() const
{
  cpp::World world;

  cpp::Group group = buildGroup();
  cpp::Instance groupInstance(group);
  groupInstance.commit();

  box3f bbox = group.getBounds<box3f>();
  bbox.lower -= .4f;
  bbox.upper += .4f;

  cpp::Instance planeInstance = makeGroundPlane(
      bbox, vec4f(.9f, .9f, .9f, 1.f), vec4f(.2f, .2f, .2f, 1.f));
  planeInstance.setParam(
      "xfm", affine3f::rotate(vec3f(-1.f, 0.f, 0.f), half_pi));
  planeInstance.commit();

  std::vector<cpp::Instance> instances;
  instances.push_back(groupInstance);
  instances.push_back(planeInstance);

  world.setParam("instance", cpp::CopiedData(instances));

  // Lights
  {
    std::vector<cpp::Light> lightHandles;

    cpp::Light sphereLight("sphere");
    sphereLight.setParam("intensity", 500.f);
    sphereLight.setParam("position", vec3f(-15.f, 15.f, -20.f));
    sphereLight.setParam("radius", 5.f);
    sphereLight.commit();

    cpp::Light ambientLight("ambient");
    ambientLight.setParam("intensity", 0.4f);
    ambientLight.setParam("color", vec3f(1.f));
    ambientLight.commit();

    lightHandles.push_back(sphereLight);
    lightHandles.push_back(ambientLight);

    world.setParam("light", cpp::CopiedData(lightHandles));
  }

  return world;
}

} // namespace testing
} // namespace ospray
