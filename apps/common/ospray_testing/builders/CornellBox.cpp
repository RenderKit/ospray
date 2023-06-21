// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
// stl
#include <random>

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct CornellBox : public detail::Builder
{
  CornellBox() = default;
  ~CornellBox() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;
};

// quad mesh data
static std::vector<vec3f> vertices = {
    // Floor
    {1.00f, -1.00f, -1.00f},
    {-1.00f, -1.00f, -1.00f},
    {-1.00f, -1.00f, 1.00f},
    {1.00f, -1.00f, 1.00f},
    // Ceiling
    {1.00f, 1.00f, -1.00f},
    {1.00f, 1.00f, 1.00f},
    {-1.00f, 1.00f, 1.00f},
    {-1.00f, 1.00f, -1.00f},
    // Backwall
    {1.00f, -1.00f, 1.00f},
    {-1.00f, -1.00f, 1.00f},
    {-1.00f, 1.00f, 1.00f},
    {1.00f, 1.00f, 1.00f},
    // RightWall
    {-1.00f, -1.00f, 1.00f},
    {-1.00f, -1.00f, -1.00f},
    {-1.00f, 1.00f, -1.00f},
    {-1.00f, 1.00f, 1.00f},
    // LeftWall
    {1.00f, -1.00f, -1.00f},
    {1.00f, -1.00f, 1.00f},
    {1.00f, 1.00f, 1.00f},
    {1.00f, 1.00f, -1.00f},
    // ShortBox Top Face
    {-0.53f, -0.40f, -0.75f},
    {-0.70f, -0.40f, -0.17f},
    {-0.13f, -0.40f, -0.00f},
    {0.05f, -0.40f, -0.57f},
    // ShortBox Left Face
    {0.05f, -1.00f, -0.57f},
    {0.05f, -0.40f, -0.57f},
    {-0.13f, -0.40f, -0.00f},
    {-0.13f, -1.00f, -0.00f},
    // ShortBox Front Face
    {-0.53f, -1.00f, -0.75f},
    {-0.53f, -0.40f, -0.75f},
    {0.05f, -0.40f, -0.57f},
    {0.05f, -1.00f, -0.57f},
    // ShortBox Right Face
    {-0.70f, -1.00f, -0.17f},
    {-0.70f, -0.40f, -0.17f},
    {-0.53f, -0.40f, -0.75f},
    {-0.53f, -1.00f, -0.75f},
    // ShortBox Back Face
    {-0.13f, -1.00f, -0.00f},
    {-0.13f, -0.40f, -0.00f},
    {-0.70f, -0.40f, -0.17f},
    {-0.70f, -1.00f, -0.17f},
    // ShortBox Bottom Face
    {-0.53f, -1.00f, -0.75f},
    {-0.70f, -1.00f, -0.17f},
    {-0.13f, -1.00f, -0.00f},
    {0.05f, -1.00f, -0.57f},
    // TallBox Top Face
    {0.53f, 0.20f, -0.09f},
    {-0.04f, 0.20f, 0.09f},
    {0.14f, 0.20f, 0.67f},
    {0.71f, 0.20f, 0.49f},
    // TallBox Left Face
    {0.53f, -1.00f, -0.09f},
    {0.53f, 0.20f, -0.09f},
    {0.71f, 0.20f, 0.49f},
    {0.71f, -1.00f, 0.49f},
    // TallBox Front Face
    {0.71f, -1.00f, 0.49f},
    {0.71f, 0.20f, 0.49f},
    {0.14f, 0.20f, 0.67f},
    {0.14f, -1.00f, 0.67f},
    // TallBox Right Face
    {0.14f, -1.00f, 0.67f},
    {0.14f, 0.20f, 0.67f},
    {-0.04f, 0.20f, 0.09f},
    {-0.04f, -1.00f, 0.09f},
    // TallBox Back Face
    {-0.04f, -1.00f, 0.09f},
    {-0.04f, 0.20f, 0.09f},
    {0.53f, 0.20f, -0.09f},
    {0.53f, -1.00f, -0.09f},
    // TallBox Bottom Face
    {0.53f, -1.00f, -0.09f},
    {-0.04f, -1.00f, 0.09f},
    {0.14f, -1.00f, 0.67f},
    {0.71f, -1.00f, 0.49f}};

static std::vector<vec4f> colors = {
    // Floor
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // Ceiling
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // Backwall
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // RightWall
    {0.140f, 0.450f, 0.091f, 1.0f},
    {0.140f, 0.450f, 0.091f, 1.0f},
    {0.140f, 0.450f, 0.091f, 1.0f},
    {0.140f, 0.450f, 0.091f, 1.0f},
    // LeftWall
    {0.630f, 0.065f, 0.05f, 1.0f},
    {0.630f, 0.065f, 0.05f, 1.0f},
    {0.630f, 0.065f, 0.05f, 1.0f},
    {0.630f, 0.065f, 0.05f, 1.0f},
    // ShortBox Top Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // ShortBox Left Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // ShortBox Front Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // ShortBox Right Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // ShortBox Back Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // ShortBox Bottom Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Top Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Left Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Front Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Right Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Back Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    // TallBox Bottom Face
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f},
    {0.725f, 0.710f, 0.68f, 1.0f}};

// Inlined definitions ////////////////////////////////////////////////////

void CornellBox::commit()
{
  Builder::commit();
  addPlane = false;
}

cpp::Group CornellBox::buildGroup() const
{
  cpp::Geometry quadMesh("mesh");

  quadMesh.setParam("vertex.position", cpp::CopiedData(vertices));
  quadMesh.setParam("vertex.color", cpp::CopiedData(colors));
  quadMesh.setParam("quadSoup", true);
  quadMesh.commit();

  cpp::GeometricModel model(quadMesh);

  // create and setup a material
  if (rendererType == "pathtracer" || rendererType == "scivis"
      || rendererType == "ao") {
    cpp::Material quadMeshMaterial(rendererType, "obj");
    quadMeshMaterial.commit();
    model.setParam("material", quadMeshMaterial);
  }

  // Put the mesh and material into a model
  model.commit();

  cpp::Group group;

  group.setParam("geometry", cpp::CopiedData(model));
  group.commit();

  return group;
}

cpp::World CornellBox::buildWorld() const
{
  auto world = Builder::buildWorld();

  cpp::Light light("quad");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 47.f);
  light.setParam("position", vec3f(-0.23f, 0.98f, -0.16f));
  light.setParam("edge1", vec3f(0.47f, 0.0f, 0.0f));
  light.setParam("edge2", vec3f(0.0f, 0.0f, 0.38f));

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(CornellBox, cornell_box);

} // namespace testing
} // namespace ospray
