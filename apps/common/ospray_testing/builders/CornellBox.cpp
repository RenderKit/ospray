// Copyright 2009-2020 Intel Corporation
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

  quadMesh.setParam("vertex.position", cpp::CopiedData(vertices));
  quadMesh.setParam("vertex.color", cpp::CopiedData(colors));
  quadMesh.setParam("index", cpp::CopiedData(indices));
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
  light1d.setParam("intensityDistribution", cpp::CopiedData(lid1d, 7));
  light1d.commit();

  cpp::Light light2d("spot");
  light2d.setParam("intensity", 1.f);
  light2d.setParam("position", vec3f(0.3f, 0.6f, 0.f));
  light2d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light2d.setParam("openingAngle", 270.f);
  light2d.setParam("penumbraAngle", 10.f);
  float lid2d[60] = {
      1.5f, 5.0f, 6.0f, 0.3f, 0.01f, 0.15f, 0.5f, 1.6f, 0.1f, 0.01f};
  light2d.setParam("intensityDistribution", cpp::CopiedData(lid2d, vec2ul(5, 12)));
  light2d.setParam("c0", vec3f(1.0f, 0.0f, 0.0f));
  light2d.commit();

  std::vector<cpp::Light> lights;
  lights.clear();
  lights.push_back(light1d);
  lights.push_back(light2d);
  world.setParam("light", cpp::CopiedData(lights));

  return world;
}

struct CornellBoxPhotometric10 : public CornellBox
{
  CornellBoxPhotometric10() = default;
  ~CornellBoxPhotometric10() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxPhotometric10::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light1d("spot");
  light1d.setParam("intensity", 5.f);
  light1d.setParam("position", vec3f(-0.6f, 0.8f, -0.5f));
  light1d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light1d.setParam("openingAngle", 360.f);
  light1d.setParam("penumbraAngle", 0.f);
  light1d.setParam("radius", 0.1f);
  float lid1d[] = {2.5f, 0.4f, 0.2f, 0.1f, 0.03f, 0.01f, 0.01f};
  light1d.setParam("intensityDistribution", cpp::CopiedData(lid1d, 7));
  light1d.commit();

  cpp::Light light2d("spot");
  light2d.setParam("intensity", 1.f);
  light2d.setParam("position", vec3f(0.3f, 0.6f, 0.f));
  light2d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light2d.setParam("openingAngle", 270.f);
  light2d.setParam("penumbraAngle", 10.f);
  light2d.setParam("radius", 0.1f);
  float lid2d[60] = {
      1.5f, 5.0f, 6.0f, 0.3f, 0.01f, 0.15f, 0.5f, 1.6f, 0.1f, 0.01f};
  light2d.setParam(
      "intensityDistribution", cpp::CopiedData(lid2d, vec2ul(5, 12)));
  light2d.setParam("c0", vec3f(1.0f, 0.0f, 0.0f));
  light2d.commit();

  std::vector<cpp::Light> lights;
  lights.clear();
  lights.push_back(light1d);
  lights.push_back(light2d);
  world.setParam("light", cpp::CopiedData(lights));

  return world;
}

struct CornellBoxQuad20 : public CornellBox
{
  CornellBoxQuad20() = default;
  ~CornellBoxQuad20() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxQuad20::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("quad");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 250.f);
  light.setParam("position", vec3f(-0.10f, 0.98f, -0.10f));
  light.setParam("edge1", vec3f(0.20f, 0.0f, 0.0f));
  light.setParam("edge2", vec3f(0.0f, 0.0f, 0.20f));

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxQuad40 : public CornellBox
{
  CornellBoxQuad40() = default;
  ~CornellBoxQuad40() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxQuad40::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("quad");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 62.5f);
  light.setParam("position", vec3f(-0.20f, 0.98f, -0.20f));
  light.setParam("edge1", vec3f(0.40f, 0.0f, 0.0f));
  light.setParam("edge2", vec3f(0.0f, 0.0f, 0.40f));

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxSphere : public CornellBox
{
  CornellBoxSphere() = default;
  ~CornellBoxSphere() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSphere::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("sphere");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 2.5f);
  light.setParam("position", vec3f(0.0f, 0.48f, 0.0f));
  light.setParam("radius", 0.0f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxSphere20 : public CornellBox
{
  CornellBoxSphere20() = default;
  ~CornellBoxSphere20() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSphere20::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("sphere");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 2.5f);
  light.setParam("position", vec3f(0.0f, 0.48f, 0.0f));
  light.setParam("radius", 0.2f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxSphere30 : public CornellBox
{
  CornellBoxSphere30() = default;
  ~CornellBoxSphere30() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSphere30::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("sphere");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 2.5f);
  light.setParam("position", vec3f(0.0f, 0.48f, 0.0f));
  light.setParam("radius", 0.3f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxSpot : public CornellBox
{
  CornellBoxSpot() = default;
  ~CornellBoxSpot() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSpot::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("spot");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", 0.0f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}


struct CornellBoxSpot20 : public CornellBox
{
  CornellBoxSpot20() = default;
  ~CornellBoxSpot20() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSpot20::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("spot");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", 0.2f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxSpot40 : public CornellBox
{
  CornellBoxSpot40() = default;
  ~CornellBoxSpot40() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxSpot40::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("spot");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", 0.4f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxRing40 : public CornellBox
{
  CornellBoxRing40() = default;
  ~CornellBoxRing40() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxRing40::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("spot");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", 0.4f);
  light.setParam("innerRadius", 0.2f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

struct CornellBoxRing80 : public CornellBox
{
  CornellBoxRing80() = default;
  ~CornellBoxRing80() override = default;

  cpp::World buildWorld() const override;
};

cpp::World CornellBoxRing80::buildWorld() const
{
  auto world = CornellBox::buildWorld();

  cpp::Light light("spot");

  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", 0.8f);
  light.setParam("innerRadius", 0.7f);

  light.commit();

  world.setParam("light", cpp::CopiedData(light));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(CornellBox, cornell_box);
OSP_REGISTER_TESTING_BUILDER(CornellBoxPhotometric, cornell_box_photometric);
OSP_REGISTER_TESTING_BUILDER(CornellBoxPhotometric10, cornell_box_photometric10);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSphere, cornell_box_sphere);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSphere20, cornell_box_sphere20);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSphere30, cornell_box_sphere30);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSpot, cornell_box_spot);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSpot20, cornell_box_spot20);
OSP_REGISTER_TESTING_BUILDER(CornellBoxSpot40, cornell_box_spot40);
OSP_REGISTER_TESTING_BUILDER(CornellBoxRing40, cornell_box_ring40);
OSP_REGISTER_TESTING_BUILDER(CornellBoxRing80, cornell_box_ring80);
OSP_REGISTER_TESTING_BUILDER(CornellBoxQuad20, cornell_box_quad20);
OSP_REGISTER_TESTING_BUILDER(CornellBoxQuad40, cornell_box_quad40);

} // namespace testing
} // namespace ospray
