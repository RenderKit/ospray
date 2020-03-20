// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"

namespace ospray {
namespace testing {
namespace detail {

void Builder::commit()
{
  rendererType = getParam<std::string>("rendererType", "scivis");
  tfColorMap = getParam<std::string>("tf.colorMap", "jet");
  tfOpacityMap = getParam<std::string>("tf.opacityMap", "linear");
  randomSeed = getParam<unsigned int>("randomSeed", 0);
}

cpp::World Builder::buildWorld() const
{
  return buildWorld({});
}

cpp::World Builder::buildWorld(
    const std::vector<cpp::Instance> &instances) const
{
  cpp::World world;

  auto group = buildGroup();

  cpp::Instance instance(group);
  instance.commit();

  std::vector<cpp::Instance> inst = instances;
  inst.push_back(instance);

  if (addPlane) {
    auto bounds = group.getBounds();

    auto extents = 0.8f * length(bounds.center() - bounds.lower);

    cpp::GeometricModel plane = makeGroundPlane(extents);
    cpp::Group planeGroup;
    planeGroup.setParam("geometry", cpp::Data(plane));
    planeGroup.commit();

    cpp::Instance planeInst(planeGroup);
    planeInst.commit();

    inst.push_back(planeInst);
  }

  world.setParam("instance", cpp::Data(inst));

  cpp::Light light("ambient");
  light.setParam("visible", false);
  light.commit();

  world.setParam("light", cpp::Data(light));

  return world;
}

cpp::TransferFunction Builder::makeTransferFunction(
    const vec2f &valueRange) const
{
  cpp::TransferFunction transferFunction("piecewiseLinear");

  std::vector<vec3f> colors;
  std::vector<float> opacities;

  if (tfColorMap == "jet") {
    colors.emplace_back(0, 0, 0.562493);
    colors.emplace_back(0, 0, 1);
    colors.emplace_back(0, 1, 1);
    colors.emplace_back(0.500008, 1, 0.500008);
    colors.emplace_back(1, 1, 0);
    colors.emplace_back(1, 0, 0);
    colors.emplace_back(0.500008, 0, 0);
  } else if (tfColorMap == "rgb") {
    colors.emplace_back(0, 0, 1);
    colors.emplace_back(0, 1, 0);
    colors.emplace_back(1, 0, 0);
  } else {
    colors.emplace_back(0.f, 0.f, 0.f);
    colors.emplace_back(1.f, 1.f, 1.f);
  }

  if (tfOpacityMap == "linear") {
    opacities.emplace_back(0.f);
    opacities.emplace_back(1.f);
  }

  transferFunction.setParam("color", cpp::Data(colors));
  transferFunction.setParam("opacity", cpp::Data(opacities));
  transferFunction.setParam("valueRange", valueRange);
  transferFunction.commit();

  return transferFunction;
}

cpp::GeometricModel Builder::makeGroundPlane(float planeExtent) const
{
  cpp::Geometry planeGeometry("mesh");

  std::vector<vec3f> v_position;
  std::vector<vec3f> v_normal;
  std::vector<vec4f> v_color;
  std::vector<vec4ui> indices;

  unsigned int startingIndex = 0;

  const vec3f up = vec3f{0.f, 1.f, 0.f};
  const vec4f gray = vec4f{0.9f, 0.9f, 0.9f, 0.75f};

  v_position.emplace_back(-planeExtent, -1.f, -planeExtent);
  v_position.emplace_back(planeExtent, -1.f, -planeExtent);
  v_position.emplace_back(planeExtent, -1.f, planeExtent);
  v_position.emplace_back(-planeExtent, -1.f, planeExtent);

  v_normal.push_back(up);
  v_normal.push_back(up);
  v_normal.push_back(up);
  v_normal.push_back(up);

  v_color.push_back(gray);
  v_color.push_back(gray);
  v_color.push_back(gray);
  v_color.push_back(gray);

  indices.emplace_back(
      startingIndex, startingIndex + 1, startingIndex + 2, startingIndex + 3);

  // stripes on ground plane
  const float stripeWidth = 0.025f;
  const float paddedExtent = planeExtent + stripeWidth;
  const size_t numStripes = 10;

  const vec4f stripeColor = vec4f{1.0f, 0.1f, 0.1f, 1.f};

  for (size_t i = 0; i < numStripes; i++) {
    // the center coordinate of the stripe, either in the x or z
    // direction
    const float coord =
        -planeExtent + float(i) / float(numStripes - 1) * 2.f * planeExtent;

    // offset the stripes by an epsilon above the ground plane
    const float yLevel = -1.f + 1e-3f;

    // x-direction stripes
    startingIndex = v_position.size();

    v_position.emplace_back(-paddedExtent, yLevel, coord - stripeWidth);
    v_position.emplace_back(paddedExtent, yLevel, coord - stripeWidth);
    v_position.emplace_back(paddedExtent, yLevel, coord + stripeWidth);
    v_position.emplace_back(-paddedExtent, yLevel, coord + stripeWidth);

    v_normal.push_back(up);
    v_normal.push_back(up);
    v_normal.push_back(up);
    v_normal.push_back(up);

    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);

    indices.emplace_back(
        startingIndex, startingIndex + 1, startingIndex + 2, startingIndex + 3);

    // z-direction stripes
    startingIndex = v_position.size();

    v_position.emplace_back(coord - stripeWidth, yLevel, -paddedExtent);
    v_position.emplace_back(coord + stripeWidth, yLevel, -paddedExtent);
    v_position.emplace_back(coord + stripeWidth, yLevel, paddedExtent);
    v_position.emplace_back(coord - stripeWidth, yLevel, paddedExtent);

    v_normal.push_back(up);
    v_normal.push_back(up);
    v_normal.push_back(up);
    v_normal.push_back(up);

    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);
    v_color.push_back(stripeColor);

    indices.emplace_back(
        startingIndex, startingIndex + 1, startingIndex + 2, startingIndex + 3);
  }

  planeGeometry.setParam("vertex.position", cpp::Data(v_position));
  planeGeometry.setParam("vertex.normal", cpp::Data(v_normal));
  planeGeometry.setParam("vertex.color", cpp::Data(v_color));
  planeGeometry.setParam("index", cpp::Data(indices));

  planeGeometry.commit();

  cpp::GeometricModel model(planeGeometry);

  if (rendererType == "pathtracer" || rendererType == "scivis") {
    cpp::Material material(rendererType, "obj");
    material.commit();
    model.setParam("material", material);
  }

  model.commit();

  return model;
}

} // namespace detail
} // namespace testing
} // namespace ospray
