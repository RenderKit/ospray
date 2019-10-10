// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include "ospcommon/math/box.h"
#include "ospcommon/math/range.h"
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/multidim_index_sequence.h"

#include "ospray_testing.h"

#include "example_util.h"

#include <imgui.h>

using namespace ospcommon;

static std::string rendererType = "pathtracer";

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
      const int permutation[256] = {
          151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233,
          7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,
          23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252,
          219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,
          174, 20,  125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,
          27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230,
          220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,
          63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
          200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186,
          3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
          255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189,
          28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,
          221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,
          108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228,
          251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
          145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
          184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236,
          205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,
          215, 61,  156, 180};

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
  static inline float lerp(float t, float a, float b)
  {
    return a + t * (b - a);
  }
  static inline float grad(int hash, float x, float y, float z)
  {
    const int h   = hash & 15;
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
  }

 public:
  static float noise(vec3f q, float frequency = 8.f)
  {
    float x     = q.x * frequency;
    float y     = q.y * frequency;
    float z     = q.z * frequency;
    const int X = (int)floor(x) & 255;
    const int Y = (int)floor(y) & 255;
    const int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    const float u = smooth(x);
    const float v = smooth(y);
    const float w = smooth(z);
    const int A   = p[X] + Y;
    const int B   = p[X + 1] + Y;
    const int AA  = p[A] + Z;
    const int BA  = p[B] + Z;
    const int BB  = p[B + 1] + Z;
    const int AB  = p[A + 1] + Z;

    return lerp(
        w,
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

cpp::Geometry PlaneGeometry(const vec4f &color, const AffineSpace3f &M)
{
  const vec3f normal = xfmNormal(M, vec3f(0.f, 1.f, 0.f));
  std::vector<vec3f> positions{
      xfmPoint(M, vec3f(-1.f, 0.f, -1.f)),
      xfmPoint(M, vec3f(+1.f, 0.f, -1.f)),
      xfmPoint(M, vec3f(+1.f, 0.f, +1.f)),
      xfmPoint(M, vec3f(-1.f, 0.f, +1.f)),
  };
  std::vector<vec3f> normals{normal, normal, normal, normal};
  std::vector<vec4f> colors{color, color, color, color};
  std::vector<vec4i> indices{vec4i(0, 1, 2, 3)};

  cpp::Geometry ospGeometry("quads");

  ospGeometry.setParam(
      "vertex.position",
      cpp::Data(positions.size(), OSP_VEC3F, positions.data()));
  ospGeometry.setParam("vertex.normal",
                       cpp::Data(normals.size(), OSP_VEC3F, normals.data()));
  ospGeometry.setParam("vertex.color",
                       cpp::Data(colors.size(), OSP_VEC4F, colors.data()));
  ospGeometry.setParam("index",
                       cpp::Data(indices.size(), OSP_VEC4UI, indices.data()));

  ospGeometry.commit();

  return ospGeometry;
}

cpp::Geometry PlaneGeometry(const vec4f &color)
{
  return PlaneGeometry(color, AffineSpace3f(one));
}

cpp::Geometry PlaneGeometry(const AffineSpace3f &M)
{
  return PlaneGeometry(vec4f(0.8f, 0.8f, 0.8f, 1.f), M);
}

cpp::Geometry PlaneGeometry()
{
  return PlaneGeometry(vec4f(0.8f, 0.8f, 0.8f, 1.f), AffineSpace3f(one));
}

cpp::Geometry BoxGeometry(const box3f &box)
{
  cpp::Geometry ospGeometry("boxes");
  ospGeometry.setParam("box", cpp::Data(1, OSP_BOX3F, &box));
  ospGeometry.commit();
  return ospGeometry;
}

cpp::VolumetricModel CreateProceduralVolumetricModel(
    std::function<bool(vec3f p)> D,
    std::vector<vec3f> colors,
    std::vector<float> opacities,
    float densityScale,
    float anisotropy)
{
  vec3l dims{128};  // should be at least 2
  const float spacing = 3.f / (reduce_max(dims) - 1);
  cpp::Volume volume("structured_volume");

  // generate volume data
  auto numVoxels = dims.product();
  std::vector<float> voxels(numVoxels, 0);
  tasking::parallel_for(dims.z, [&](int64_t z) {
    for (int y = 0; y < dims.y; ++y) {
      for (int x = 0; x < dims.x; ++x) {
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

  volume.setParam("voxelData", cpp::Data(numVoxels, OSP_FLOAT, voxels.data()));
  volume.setParam("voxelType", int(OSP_FLOAT));
  volume.setParam("dimensions", vec3i(dims));
  volume.setParam("gridOrigin", vec3f(-1.5f, -1.5f, -1.5f));
  volume.setParam("gridSpacing", vec3f(spacing));
  volume.commit();

  cpp::TransferFunction tfn("piecewise_linear");
  tfn.setParam("valueRange", voxelRange.toVec2());
  tfn.setParam("color", cpp::Data(colors.size(), OSP_VEC3F, colors.data()));
  tfn.setParam("opacity",
               cpp::Data(opacities.size(), OSP_FLOAT, opacities.data()));
  tfn.commit();

  cpp::VolumetricModel volumeModel(volume);
  volumeModel.setParam("densityScale", densityScale);
  volumeModel.setParam("anisotropy", anisotropy);
  volumeModel.setParam("transferFunction", tfn);
  volumeModel.commit();

  return volumeModel;
}

cpp::GeometricModel CreateGeometricModel(cpp::Geometry geo, const vec3f &kd)
{
  cpp::GeometricModel geometricModel(geo);

  cpp::Material objMaterial(rendererType, "OBJMaterial");
  objMaterial.setParam("Kd", kd);
  objMaterial.commit();

  geometricModel.setParam("material", cpp::Data(objMaterial));

  return geometricModel;
}

cpp::Instance CreateInstance(cpp::Group group, const AffineSpace3f &xfm)
{
  cpp::Instance instance(group);

  instance.setParam("xfm", xfm);

  return instance;
}

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-r" || arg == "--renderer")
        rendererType = argv[++i];
    }

    float densityScale = 10.0f;
    float anisotropy   = 0.0f;

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
    volumetricModels.emplace_back(CreateProceduralVolumetricModel(
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
    volumetricModels.emplace_back(CreateProceduralVolumetricModel(
        [&](vec3f p) {
          vec3f X = 2.f * p - vec3f(1.f);
          return torus(
              (1.4f + 0.4 * turbulence(p, 12.f, 12)) * X, 0.75f, 0.175f);
        },
        {vec3f(0.0, 0.0, 0.0),
         vec3f(1.0, 0.65, 0.0),
         vec3f(0.12, 0.6, 1.0),
         vec3f(1.0, 1.0, 1.0)},
        {0.f, 0.33f, 0.66f, 1.f},
        densityScale,
        anisotropy));

    for (auto volumetricModel : volumetricModels)
      volumetricModel.commit();

    // create geometries
    auto box1 = BoxGeometry(
        box3f(vec3f(-1.5f, -1.f, -0.75f), vec3f(-0.5f, 0.f, 0.25f)));
    auto box2 =
        BoxGeometry(box3f(vec3f(0.0f, -1.5f, 0.f), vec3f(2.f, 1.5f, 2.f)));
    auto plane =
        PlaneGeometry(vec4f(vec3f(1.0f), 1.f),
                      AffineSpace3f::translate(vec3f(0.f, -2.5f, 0.f)) *
                          AffineSpace3f::scale(vec3f(10.f, 1.f, 10.f)));

    std::vector<cpp::GeometricModel> geometricModels;
    geometricModels.emplace_back(
        CreateGeometricModel(box1, vec3f(0.2f, 0.2f, 0.2f)));
    geometricModels.emplace_back(
        CreateGeometricModel(box2, vec3f(0.2f, 0.2f, 0.2f)));
    geometricModels.emplace_back(
        CreateGeometricModel(plane, vec3f(0.2f, 0.2f, 0.2f)));

    for (auto geometricModel : geometricModels)
      geometricModel.commit();

    cpp::World world;
    cpp::Group group_geometry;

    std::vector<cpp::Group> volumetricGroups;
    for (size_t i = 0; i < volumetricModels.size(); ++i)
      volumetricGroups.emplace_back();

    std::vector<cpp::Instance> instances;
    instances.emplace_back(CreateInstance(group_geometry, one));

    if (volumetricGroups.size() > 0) {
      instances.emplace_back(
          CreateInstance(volumetricGroups[0],
                         AffineSpace3f::translate(vec3f(0.f, -0.5f, 0.f)) *
                             AffineSpace3f::scale(vec3f(1.25f))));
    }

    if (volumetricGroups.size() > 1) {
      instances.emplace_back(
          CreateInstance(volumetricGroups[1],
                         AffineSpace3f::translate(vec3f(0.f, -0.5f, 0.f)) *
                             AffineSpace3f::scale(vec3f(2.5))));
    }

    for (auto instance : instances)
      instance.commit();

    world.setParam("instance",
                   cpp::Data(instances.size(), OSP_INSTANCE, instances.data()));

    // create OSPRay renderer
    int maxDepth = 5;

    cpp::Renderer renderer(rendererType);
    renderer.setParam("maxDepth", maxDepth);
    renderer.setParam("rouletteDepth", 32);
    renderer.setParam("minContribution", 0.f);

    std::vector<cpp::Light> light_handles;
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
    ambientLight.commit();

    bool showVolume         = true;
    bool showSphere         = true;
    bool showTorus          = false;
    bool showGeometry       = false;
    bool enableQuadLight    = true;
    bool enableAmbientLight = false;

    auto updateScene = [&]() {
      group_geometry.removeParam("geometry");

      for (auto group : volumetricGroups)
        group.removeParam("volume");

      if (showVolume) {
        for (size_t i = 0; i < volumetricGroups.size(); ++i) {
          if ((!showTorus && i == 1) || (!showSphere && i == 0))
            continue;

          volumetricGroups[i].setParam(
              "volume",
              cpp::Data(1, OSP_VOLUMETRIC_MODEL, &volumetricModels[i]));
        }
      }

      if (showGeometry) {
        group_geometry.setParam("geometry",
                                cpp::Data(geometricModels.size(),
                                          OSP_GEOMETRIC_MODEL,
                                          geometricModels.data()));
      }

      light_handles.clear();

      if (enableQuadLight)
        light_handles.push_back(quadLight);
      if (enableAmbientLight)
        light_handles.push_back(ambientLight);

      if (light_handles.empty())
        world.removeParam("light");
      else {
        world.setParam(
            "light",
            cpp::Data(light_handles.size(), OSP_LIGHT, light_handles.data()));
      }
    };

    updateScene();

    group_geometry.commit();

    for (auto group : volumetricGroups)
      group.commit();

    world.commit();

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
        new GLFWOSPRayWindow(vec2i(1024, 768), world, renderer));

    // ImGui //

    glfwOSPRayWindow->registerImGuiCallback([&]() {
      bool updateWorld = false;
      bool commitWorld = false;

      if (ImGui::SliderInt("maxDepth", &maxDepth, 0, 128)) {
        commitWorld = true;
        renderer.setParam("maxDepth", maxDepth);
        glfwOSPRayWindow->addObjectToCommit(renderer.handle());
      }

      if (ImGui::Checkbox("Show Volumes", &showVolume))
        updateWorld = true;
      if (ImGui::Checkbox("Show Sphere", &showSphere))
        updateWorld = true;
      if (ImGui::Checkbox("Show Torus", &showTorus))
        updateWorld = true;
      if (ImGui::Checkbox("Show Geometry", &showGeometry))
        updateWorld = true;
      if (ImGui::Checkbox("Quad Light", &enableQuadLight))
        updateWorld = true;
      if (ImGui::Checkbox("Ambient Light", &enableAmbientLight))
        updateWorld = true;

      commitWorld = updateWorld;

      if (ImGui::SliderFloat("densityScale", &densityScale, 0.f, 50.f)) {
        commitWorld = true;
        for (auto vModel : volumetricModels) {
          vModel.setParam("densityScale", densityScale);
          glfwOSPRayWindow->addObjectToCommit(vModel.handle());
        }
      }

      if (ImGui::SliderFloat("anisotropy", &anisotropy, -1.f, 1.f)) {
        commitWorld = true;
        for (auto vModel : volumetricModels) {
          vModel.setParam("anisotropy", anisotropy);
          glfwOSPRayWindow->addObjectToCommit(vModel.handle());
        }
      }

      if (updateWorld)
        updateScene();

      if (commitWorld) {
        glfwOSPRayWindow->addObjectToCommit(group_geometry.handle());
        for (auto group : volumetricGroups)
          glfwOSPRayWindow->addObjectToCommit(group.handle());
        glfwOSPRayWindow->addObjectToCommit(world.handle());
        glfwOSPRayWindow->addObjectToCommit(renderer.handle());
      }
    });

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();
  }

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
