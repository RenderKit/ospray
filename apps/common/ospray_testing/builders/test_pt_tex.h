// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct PtTex : public detail::Builder
{
  PtTex() = default;
  ~PtTex() override = default;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;

  virtual std::vector<cpp::Material> buildMaterials(
      const cpp::Texture &texR, const cpp::Texture &texRGBA) const = 0;
};

// Inlined definitions ////////////////////////////////////////////////////

static cpp::Texture makeCheckerboardTextureR8()
{
  // Prepare pixel data
  constexpr uint32_t width = 16;
  constexpr uint32_t height = 16;
  std::array<uint8_t, width * height> dbyte;
  index_sequence_2D idx(vec2i(width, height));
  for (auto i : idx)
    dbyte[idx.flatten(i)] = ((i.x & 2) ^ (i.y & 2)) * 255;

  // Create texture object
  cpp::Texture tex("texture2d");
  tex.setParam("format", OSP_TEXTURE_R8);
  tex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
  tex.setParam("data", cpp::CopiedData(dbyte.data(), vec2ul(width, height)));
  tex.commit();

  // Ready
  return tex;
}

static cpp::Texture makeCheckerboardTextureRGBA8()
{
  // Prepare pixel data
  constexpr uint32_t width = 16;
  constexpr uint32_t height = 16;
  std::array<vec4uc, width * height> dbyte;
  index_sequence_2D idx(vec2i(width, height));
  for (auto i : idx) {
    uint8_t v = ((i.x & 2) ^ (i.y & 2)) * 255;
    dbyte[idx.flatten(i)] = vec4uc(v, v, v, 128 + v / 2);
  }

  // Create texture object
  cpp::Texture tex("texture2d");
  tex.setParam("format", OSP_TEXTURE_RGBA8);
  tex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
  tex.setParam("data", cpp::CopiedData(dbyte.data(), vec2ul(width, height)));
  tex.commit();

  // Ready
  return tex;
}

static std::vector<vec3f> makeSphereVertices(
    uint32_t latitudeCount, uint32_t longitudeCount, float radius)
{
  // Calculate latitude and longitude angular steps (in radians)
  std::vector<vec3f> vPos;
  const float latAngleStep = float(pi) / float(latitudeCount);
  const float longAngleStep = (2.0f * float(pi)) / float(longitudeCount);

  // Iterate through latitude segments
  for (uint32_t latitudeId = 0; latitudeId <= latitudeCount; latitudeId++) {
    // Calculate longitude radius at current latitude
    float longRadius = radius * sin(latAngleStep * latitudeId);

    // Determine segment level
    float y = radius * cos(latAngleStep * latitudeId);

    // Iterate through longitude segments
    for (uint32_t longitudeId = 0; longitudeId <= longitudeCount;
         longitudeId++) {
      // Calculate vertex position
      float x = longRadius * cos(longAngleStep * longitudeId);
      float z = longRadius * sin(longAngleStep * longitudeId);
      vPos.push_back(vec3f(x, y, z));
    }
  }

  // Ready
  return vPos;
}

static std::vector<vec2f> makeSphereTexCoords(uint32_t latitudeCount,
    uint32_t longitudeCount,
    float tileLat,
    float tileLong)
{
  // Iterate through latitude segments
  std::vector<vec2f> vTexCoord;
  for (uint32_t latitudeId = 0; latitudeId <= latitudeCount; latitudeId++) {
    // Iterate through longitude segments
    for (uint32_t longitudeId = 0; longitudeId <= longitudeCount;
         longitudeId++) {
      // Calculate texture coordinates
      vTexCoord.push_back(vec2f(tileLong * longitudeId / float(longitudeCount),
          tileLat * (latitudeId - 1) / float(latitudeCount)));
    }
  }

  // Ready
  return vTexCoord;
}

static std::vector<vec3ui> makeSphereIndices(
    uint32_t latitudeCount, uint32_t longitudeCount, bool outside)
{
  // Iterate through latitude segments
  std::vector<vec3ui> indices;
  for (uint32_t latitudeId = 0; latitudeId < latitudeCount; latitudeId++) {
    // Calculate indices of segment left-most vertices
    uint32_t firstVertex1 = latitudeId * (longitudeCount + 1);
    uint32_t firstVertex2 = (latitudeId + 1) * (longitudeCount + 1);

    // Iterate through longitude segments
    for (uint32_t longitudeId = 0; longitudeId < longitudeCount;
         longitudeId++) {
      // Setup first triangle indices
      uint32_t i2 = firstVertex2 + longitudeId + 1;
      uint32_t i3 = firstVertex2 + longitudeId;
      indices.push_back(vec3ui(firstVertex1 + longitudeId,
          (outside) ? i2 : i3,
          (outside) ? i3 : i2));

      // Setup second triangle indices
      i2 = firstVertex1 + longitudeId + 1;
      i3 = firstVertex2 + longitudeId + 1;
      indices.push_back(vec3ui(firstVertex1 + longitudeId,
          (outside) ? i2 : i3,
          (outside) ? i3 : i2));
    }
  }

  // Ready
  return indices;
}

static cpp::Geometry makeSphereMesh(
    uint32_t latitudeCount, uint32_t longitudeCount)
{
  cpp::Geometry sphereMesh("mesh");

  // Prepare mesh buffers
  std::vector<vec3f> vertices =
      makeSphereVertices(latitudeCount, longitudeCount, 1.f);
  std::vector<vec2f> texCoords =
      makeSphereTexCoords(latitudeCount, longitudeCount, 1.f, 2.f);
  std::vector<vec3ui> indices =
      makeSphereIndices(latitudeCount, longitudeCount, false);

  // Set mesh parameters
  sphereMesh.setParam("vertex.position", cpp::CopiedData(vertices));
  sphereMesh.setParam("vertex.normal", cpp::CopiedData(vertices));
  sphereMesh.setParam("vertex.texcoord", cpp::CopiedData(texCoords));
  sphereMesh.setParam("index", cpp::CopiedData(indices));
  sphereMesh.commit();

  // Ready
  return sphereMesh;
}

template <typename T>
void setTexture(cpp::Material &mat,
    const std::string &name,
    const cpp::Texture &tex,
    const T mul = T(1.f))
{
  mat.setParam(name, mul);
  mat.setParam("map_" + name, tex);
  mat.commit();
}

inline cpp::Group PtTex::buildGroup() const
{
  // Do nothing, needs per-instance materials feature to have single group
  return cpp::Group();
}

inline cpp::World PtTex::buildWorld() const
{
  cpp::World world;
  std::vector<cpp::Instance> instances;

  cpp::Texture texR = makeCheckerboardTextureR8();
  cpp::Texture texRGBA = makeCheckerboardTextureRGBA8();

  std::vector<cpp::Material> materials = buildMaterials(texR, texRGBA);
  size_t dimCount = sqrtf(materials.size());
  if (dimCount * dimCount < materials.size())
    dimCount++;
  const size_t dimSize = dimCount - 1;

  cpp::Geometry sphereGeometry = makeSphereMesh(25, 50);

  // Note: this is a temporary implementation, per-instance materials feature
  // should be used here
  box3f bbox(empty);
  index_sequence_2D numSpheres(dimCount);
  for (auto i : numSpheres) {
    const auto idx = numSpheres.flatten(i);
    if (idx >= materials.size())
      break;
    // Construct GeometricModel for each instance
    cpp::GeometricModel model(sphereGeometry);
    model.setParam("material", cpp::CopiedData(materials[idx]));
    model.commit();

    // Construct Group for each instance
    cpp::Group group;
    group.setParam("geometry", cpp::CopiedData(model));
    group.commit();

    // Construct instance
    cpp::Instance instance(group);
    auto i_f = static_cast<vec2f>(i);
    instance.setParam("xfm",
        affine3f::translate(
            vec3f(dimSize / 2.f - i_f.x, dimSize / 2.f - i_f.y, .6f))
            * affine3f::scale(.4f));
    instance.commit();
    instances.push_back(instance);
    bbox.extend(instance.getBounds<box3f>());
  }

  bbox.lower -= .4f;
  bbox.upper += .4f;
  cpp::Instance planeInstance = makeGroundPlane(
      bbox, vec4f(.9f, .9f, .9f, 1.f), vec4f(.2f, .2f, .2f, 1.f));
  planeInstance.setParam(
      "xfm", affine3f::rotate(vec3f(-1.f, 0.f, 0.f), half_pi));
  planeInstance.commit();
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
