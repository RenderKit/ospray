// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// stl
#include <functional>

using namespace ospcommon::math;

namespace ospray {
namespace testing {

struct PerlinNoiseVolumes : public detail::Builder
{
  PerlinNoiseVolumes() = default;
  ~PerlinNoiseVolumes() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;

 private:
  bool addSphereVolume{true};
  bool addTorusVolume{false};
  bool addBoxes{false};

  bool addAreaLight{true};
  bool addAmbientLight{false};

  float densityScale{10.f};
  float anisotropy{0.f};
};

/* heavily based on Perlin's Java reference implementation of
 * the improved perlin noise paper from Siggraph 2002 from here
 * https://mrl.nyu.edu/~perlin/noise/
 **/
class PerlinNoise
{
  struct PerlinNoiseData
  {
    PerlinNoiseData()
    {
      const int permutation[256] = {151,
          160,
          137,
          91,
          90,
          15,
          131,
          13,
          201,
          95,
          96,
          53,
          194,
          233,
          7,
          225,
          140,
          36,
          103,
          30,
          69,
          142,
          8,
          99,
          37,
          240,
          21,
          10,
          23,
          190,
          6,
          148,
          247,
          120,
          234,
          75,
          0,
          26,
          197,
          62,
          94,
          252,
          219,
          203,
          117,
          35,
          11,
          32,
          57,
          177,
          33,
          88,
          237,
          149,
          56,
          87,
          174,
          20,
          125,
          136,
          171,
          168,
          68,
          175,
          74,
          165,
          71,
          134,
          139,
          48,
          27,
          166,
          77,
          146,
          158,
          231,
          83,
          111,
          229,
          122,
          60,
          211,
          133,
          230,
          220,
          105,
          92,
          41,
          55,
          46,
          245,
          40,
          244,
          102,
          143,
          54,
          65,
          25,
          63,
          161,
          1,
          216,
          80,
          73,
          209,
          76,
          132,
          187,
          208,
          89,
          18,
          169,
          200,
          196,
          135,
          130,
          116,
          188,
          159,
          86,
          164,
          100,
          109,
          198,
          173,
          186,
          3,
          64,
          52,
          217,
          226,
          250,
          124,
          123,
          5,
          202,
          38,
          147,
          118,
          126,
          255,
          82,
          85,
          212,
          207,
          206,
          59,
          227,
          47,
          16,
          58,
          17,
          182,
          189,
          28,
          42,
          223,
          183,
          170,
          213,
          119,
          248,
          152,
          2,
          44,
          154,
          163,
          70,
          221,
          153,
          101,
          155,
          167,
          43,
          172,
          9,
          129,
          22,
          39,
          253,
          19,
          98,
          108,
          110,
          79,
          113,
          224,
          232,
          178,
          185,
          112,
          104,
          218,
          246,
          97,
          228,
          251,
          34,
          242,
          193,
          238,
          210,
          144,
          12,
          191,
          179,
          162,
          241,
          81,
          51,
          145,
          235,
          249,
          14,
          239,
          107,
          49,
          192,
          214,
          31,
          181,
          199,
          106,
          157,
          184,
          84,
          204,
          176,
          115,
          121,
          50,
          45,
          127,
          4,
          150,
          254,
          138,
          236,
          205,
          93,
          222,
          114,
          67,
          29,
          24,
          72,
          243,
          141,
          128,
          195,
          78,
          66,
          215,
          61,
          156,
          180};

      for (int i = 0; i < 256; i++)
        p[256 + i] = p[i] = permutation[i];
    }
    inline int operator[](size_t idx) const
    {
      return p[idx];
    }
    int p[512];
  };

  static PerlinNoiseData p;

  static inline float smooth(float t)
  {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
  }

  static inline float grad(int hash, float x, float y, float z)
  {
    const int h = hash & 15;
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
  }

 public:
  static float noise(vec3f q, float frequency = 8.f)
  {
    float x = q.x * frequency;
    float y = q.y * frequency;
    float z = q.z * frequency;
    const int X = (int)floor(x) & 255;
    const int Y = (int)floor(y) & 255;
    const int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    const float u = smooth(x);
    const float v = smooth(y);
    const float w = smooth(z);
    const int A = p[X] + Y;
    const int B = p[X + 1] + Y;
    const int AA = p[A] + Z;
    const int BA = p[B] + Z;
    const int BB = p[B + 1] + Z;
    const int AB = p[A + 1] + Z;

    return lerp(w,
        lerp(v,
            lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
            lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
        lerp(v,
            lerp(u,
                grad(p[AA + 1], x, y, z - 1),
                grad(p[BA + 1], x - 1, y, z - 1)),
            lerp(u,
                grad(p[AB + 1], x, y - 1, z - 1),
                grad(p[BB + 1], x - 1, y - 1, z - 1))));
  }
};

PerlinNoise::PerlinNoiseData PerlinNoise::p;

// Inlined definitions ////////////////////////////////////////////////////

// Helper functions //

cpp::Geometry makeBoxGeometry(const box3f &box)
{
  cpp::Geometry ospGeometry("box");
  ospGeometry.setParam("box", cpp::Data(1, OSP_BOX3F, &box));
  ospGeometry.commit();
  return ospGeometry;
}

cpp::VolumetricModel createProceduralVolumetricModel(
    std::function<bool(vec3f p)> D,
    std::vector<vec3f> colors,
    std::vector<float> opacities,
    float densityScale,
    float anisotropy)
{
  vec3ul dims{128}; // should be at least 2
  const float spacing = 3.f / (reduce_max(dims) - 1);
  cpp::Volume volume("structuredRegular");

  // generate volume data
  auto numVoxels = dims.product();
  std::vector<float> voxels(numVoxels, 0);
  tasking::parallel_for(dims.z, [&](uint64_t z) {
    for (uint64_t y = 0; y < dims.y; ++y) {
      for (uint64_t x = 0; x < dims.x; ++x) {
        vec3f p = vec3f(x + 0.5f, y + 0.5f, z + 0.5f) / dims;
        if (D(p))
          voxels[dims.x * dims.y * z + dims.x * y + x] =
              0.5f + 0.5f * PerlinNoise::noise(p, 12);
      }
    }
  });

  // calculate voxel range
  range1f voxelRange;
  std::for_each(voxels.begin(), voxels.end(), [&](float &v) {
    if (!std::isnan(v))
      voxelRange.extend(v);
  });

  volume.setParam("data", cpp::Data(dims, voxels.data()));
  volume.setParam("gridOrigin", vec3f(-1.5f, -1.5f, -1.5f));
  volume.setParam("gridSpacing", vec3f(spacing));
  volume.commit();

  cpp::TransferFunction tfn("piecewiseLinear");
  tfn.setParam("valueRange", voxelRange.toVec2());
  tfn.setParam("color", cpp::Data(colors));
  tfn.setParam("opacity", cpp::Data(opacities));
  tfn.commit();

  cpp::VolumetricModel volumeModel(volume);
  volumeModel.setParam("densityScale", densityScale);
  volumeModel.setParam("anisotropy", anisotropy);
  volumeModel.setParam("transferFunction", tfn);
  volumeModel.commit();

  return volumeModel;
}

cpp::GeometricModel createGeometricModel(
    cpp::Geometry geo, const std::string &rendererType, const vec3f &kd)
{
  cpp::GeometricModel geometricModel(geo);

  if (rendererType == "pathtracer" || rendererType == "scivis") {
    cpp::Material objMaterial(rendererType, "obj");
    objMaterial.setParam("kd", kd);
    objMaterial.commit();
    geometricModel.setParam("material", objMaterial);
  }

  return geometricModel;
}

// PerlineNoiseVolumes definitions //

void PerlinNoiseVolumes::commit()
{
  Builder::commit();

  addSphereVolume = getParam<bool>("addSphereVolume", true);
  addTorusVolume = getParam<bool>("addTorusVolume", true);
  addBoxes = getParam<bool>("addBoxes", false);

  addAreaLight = getParam<bool>("addAreaLight", true);
  addAmbientLight = getParam<bool>("addAmbientLight", true);

  densityScale = getParam<float>("densityScale", 10.f);
  anisotropy = getParam<float>("anisotropy", 0.f);
}

cpp::Group PerlinNoiseVolumes::buildGroup() const
{
  cpp::Group group;

  auto turbulence = [](const vec3f &p, float base_freqency, int octaves) {
    float value = 0.f;
    float scale = 1.f;
    for (int o = 0; o < octaves; ++o) {
      value += PerlinNoise::noise(scale * p, base_freqency) / scale;
      scale *= 2.f;
    }
    return value;
  };

  auto torus = [](vec3f X, float R, float r) -> bool {
    const float tmp = sqrtf(X.x * X.x + X.z * X.z) - R;
    return tmp * tmp + X.y * X.y - r * r < 0.f;
  };

  std::vector<cpp::VolumetricModel> volumetricModels;

  if (addSphereVolume) {
    volumetricModels.emplace_back(createProceduralVolumetricModel(
        [&](vec3f p) {
          vec3f X = 2.f * p - vec3f(1.f);
          return length((1.4f + 0.4 * turbulence(p, 12.f, 12)) * X) < 1.f;
        },
        {vec3f(0.f, 0.0f, 0.f),
            vec3f(1.f, 0.f, 0.f),
            vec3f(0.f, 1.f, 1.f),
            vec3f(1.f, 1.f, 1.f)},
        {0.f, 0.33f, 0.66f, 1.f},
        densityScale,
        anisotropy));
  }

  if (addTorusVolume) {
    volumetricModels.emplace_back(createProceduralVolumetricModel(
        [&](vec3f p) {
          vec3f X = 2.f * p - vec3f(1.f);
          return torus(
              (1.4f + 0.4 * turbulence(p, 12.f, 12)) * X, 1.0f, 0.375f);
        },
        {vec3f(0.0, 0.0, 0.0),
            vec3f(1.0, 0.65, 0.0),
            vec3f(0.12, 0.6, 1.0),
            vec3f(1.0, 1.0, 1.0)},
        {0.f, 0.33f, 0.66f, 1.f},
        densityScale,
        anisotropy));
  }

  for (auto volumetricModel : volumetricModels)
    volumetricModel.commit();

  std::vector<cpp::GeometricModel> geometricModels;

  if (addBoxes) {
    auto box1 = makeBoxGeometry(
        box3f(vec3f(-1.5f, -1.f, -0.75f), vec3f(-0.5f, 0.f, 0.25f)));
    auto box2 =
        makeBoxGeometry(box3f(vec3f(0.0f, -1.5f, 0.f), vec3f(2.f, 1.5f, 2.f)));

    geometricModels.emplace_back(
        createGeometricModel(box1, rendererType, vec3f(0.2f, 0.2f, 0.2f)));
    geometricModels.emplace_back(
        createGeometricModel(box2, rendererType, vec3f(0.2f, 0.2f, 0.2f)));
  }

  for (auto geometricModel : geometricModels)
    geometricModel.commit();

  if (!volumetricModels.empty())
    group.setParam("volume", cpp::Data(volumetricModels));

  if (!geometricModels.empty())
    group.setParam("geometry", cpp::Data(geometricModels));

  group.commit();

  return group;
}

cpp::World PerlinNoiseVolumes::buildWorld() const
{
  auto world = Builder::buildWorld();

  std::vector<cpp::Light> lightHandles;

  cpp::Light quadLight("quad");
  quadLight.setParam("position", vec3f(-4.0f, 3.0f, 1.0f));
  quadLight.setParam("edge1", vec3f(0.f, 0.0f, -1.0f));
  quadLight.setParam("edge2", vec3f(1.0f, 0.5, 0.0f));
  quadLight.setParam("intensity", 50.0f);
  quadLight.setParam("color", vec3f(2.6f, 2.5f, 2.3f));
  quadLight.commit();

  cpp::Light ambientLight("ambient");
  ambientLight.setParam("intensity", 0.4f);
  ambientLight.setParam("color", vec3f(1.f));
  ambientLight.setParam("visible", false);
  ambientLight.commit();

  if (addAreaLight)
    lightHandles.push_back(quadLight);
  if (addAmbientLight)
    lightHandles.push_back(ambientLight);

  if (lightHandles.empty())
    world.removeParam("light");
  else
    world.setParam("light", cpp::Data(lightHandles));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(PerlinNoiseVolumes, perlin_noise_volumes);

} // namespace testing
} // namespace ospray
