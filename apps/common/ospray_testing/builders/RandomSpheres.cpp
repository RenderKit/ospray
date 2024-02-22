// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
#include "rkcommon/utility/random.h"
// stl
#include <random>

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct Spheres : public detail::Builder
{
  Spheres(OSPSphereType type);
  ~Spheres() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;

 private:
  // Data //

  OSPSphereType sphereType;
  int numSpheres{100};
};

// Inlined definitions ////////////////////////////////////////////////////

Spheres::Spheres(OSPSphereType type) : sphereType(type) {}

void Spheres::commit()
{
  Builder::commit();

  numSpheres = getParam<int>("numSpheres", 100);
}

cpp::Group Spheres::buildGroup() const
{
  std::vector<vec3f> s_center(numSpheres);
  std::vector<float> s_radius(numSpheres);
  std::vector<vec4f> s_colors(numSpheres);

  // create random number distributions for sphere center, radius, and color
  std::mt19937 gen(randomSeed);

  utility::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
  utility::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);
  utility::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

  // populate the spheres

  for (auto &center : s_center) {
    center.x = centerDistribution(gen);
    center.y = centerDistribution(gen);
    center.z = centerDistribution(gen);
  }

  for (auto &radius : s_radius)
    radius = radiusDistribution(gen);

  for (auto &c : s_colors) {
    c.x = colorDistribution(gen);
    c.y = colorDistribution(gen);
    c.z = colorDistribution(gen);
    c.w = colorDistribution(gen);
  }

  // create the sphere geometry, and assign attributes
  cpp::Geometry spheresGeometry("sphere");

  spheresGeometry.setParam("type", sphereType);
  spheresGeometry.setParam("sphere.position", cpp::CopiedData(s_center));
  if (sphereType == OSP_ORIENTED_DISC) {
    spheresGeometry.setParam("sphere.normal", cpp::CopiedData(s_center));
  }

  spheresGeometry.setParam("sphere.radius", cpp::CopiedData(s_radius));
  spheresGeometry.commit();

  cpp::GeometricModel model(spheresGeometry);

  model.setParam("color", cpp::CopiedData(s_colors));

  if (rendererType == "pathtracer") {
    // create glass material and assign to geometry
    cpp::Material glassMaterial("thinGlass");
    glassMaterial.setParam("attenuationDistance", 0.2f);
    glassMaterial.commit();
    model.setParam("material", glassMaterial);
  } else {
    cpp::Material glassMaterial("obj");
    glassMaterial.commit();
    model.setParam("material", glassMaterial);
  }

  model.commit();

  cpp::Group group;

  group.setParam("geometry", cpp::CopiedData(model));
  group.commit();

  return group;
}

OSP_REGISTER_TESTING_BUILDER(Spheres(OSP_SPHERE), random_spheres);
OSP_REGISTER_TESTING_BUILDER(Spheres(OSP_DISC), random_discs);
OSP_REGISTER_TESTING_BUILDER(Spheres(OSP_ORIENTED_DISC), random_oriented_discs);

} // namespace testing
} // namespace ospray
