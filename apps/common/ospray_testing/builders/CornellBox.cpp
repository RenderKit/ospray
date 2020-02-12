// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
// stl
#include <random>

using namespace ospcommon::math;

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

static std::vector<vec4ui> indices = {
    {0, 1, 2, 3}, // Floor
    {4, 5, 6, 7}, // Ceiling
    {8, 9, 10, 11}, // Backwall
    {12, 13, 14, 15}, // RightWall
    {16, 17, 18, 19}, // LeftWall
    {20, 21, 22, 23}, // ShortBox Top Face
    {24, 25, 26, 27}, // ShortBox Left Face
    {28, 29, 30, 31}, // ShortBox Front Face
    {32, 33, 34, 35}, // ShortBox Right Face
    {36, 37, 38, 39}, // ShortBox Back Face
    {40, 41, 42, 43}, // ShortBox Bottom Face
    {44, 45, 46, 47}, // TallBox Top Face
    {48, 49, 50, 51}, // TallBox Left Face
    {52, 53, 54, 55}, // TallBox Front Face
    {56, 57, 58, 59}, // TallBox Right Face
    {60, 61, 62, 63}, // TallBox Back Face
    {64, 65, 66, 67} // TallBox Bottom Face
};

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

  quadMesh.setParam("vertex.position", cpp::Data(vertices));
  quadMesh.setParam("vertex.color", cpp::Data(colors));
  quadMesh.setParam("index", cpp::Data(indices));
  quadMesh.commit();

  cpp::GeometricModel model(quadMesh);

  // create and setup a material
  if (rendererType == "pathtracer" || rendererType == "scivis") {
    cpp::Material quadMeshMaterial(rendererType, "obj");
    quadMeshMaterial.commit();
    model.setParam("material", quadMeshMaterial);
  }

  // Put the mesh and material into a model
  model.commit();

  cpp::Group group;

  group.setParam("geometry", cpp::Data(model));
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

  world.setParam("light", cpp::Data(light));

  return world;
}

struct CornellBoxPhotometric : public CornellBox
{
  CornellBoxPhotometric() = default;
  ~CornellBoxPhotometric() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxPhotometric::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light1d("spot");
  light1d.setParam("intensity", 5.f);
  light1d.setParam("position", vec3f(-0.6f, 0.8f, -0.5f));
  light1d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light1d.setParam("openingAngle", 360.f);
  light1d.setParam("penumbraAngle", 0.f);
  float lid1d[] = {2.5f, 0.4f, 0.2f, 0.1f, 0.03f, 0.01f, 0.01f};
  light1d.setParam("intensityDistribution", cpp::Data(7, lid1d));
  light1d.commit();

  cpp::Light light2d("spot");
  light2d.setParam("intensity", 1.f);
  light2d.setParam("position", vec3f(0.3f, 0.6f, 0.f));
  light2d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light2d.setParam("openingAngle", 270.f);
  light2d.setParam("penumbraAngle", 10.f);
  float lid2d[60] = {
      1.5f, 5.0f, 6.0f, 0.3f, 0.01f, 0.15f, 0.5f, 1.6f, 0.1f, 0.01f};
  light2d.setParam("intensityDistribution", cpp::Data(vec2ul(5, 12), lid2d));
  light2d.setParam("c0", vec3f(1.0f, 0.0f, 0.0f));
  light2d.commit();

  std::vector<cpp::Light> lights;
  lights.clear();
  lights.push_back(light1d);
  lights.push_back(light2d);
  world.setParam("light", cpp::Data(lights));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(CornellBox, cornell_box);
OSP_REGISTER_TESTING_BUILDER(CornellBoxPhotometric, cornell_box_photometric);

} // namespace testing
} // namespace ospray
