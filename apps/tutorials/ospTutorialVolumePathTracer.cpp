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

#include "ospcommon/math/range.h"
#include "ospcommon/math/box.h"
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/multidim_index_sequence.h"

#include "ospray_testing.h"

#include "tutorial_util.h"

#include <imgui.h>

using namespace ospcommon;

static std::string renderer_type = "pathtracer";

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
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,
        37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
        57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,
        166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245, 40,244,
        102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,
        116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202, 38,147,
        118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189, 28, 42,223,183,170,213,
        119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,
        110,79,113,224,232,178,185,112,104,218,246,97,228,251, 34,242,193,238,210,144,12,
        191,179,162,241,81,51,145,235,249,14,239,107, 49,192,214, 31,181,199,106,157,184,
        84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67, 29, 24, 72,243,
        141,128,195,78,66,215,61,156,180
      };
      for (int i=0; i < 256 ; i++) p[256+i] = p[i] = permutation[i];
    }
    inline int operator[](size_t idx) const { return p[idx]; }
    int p[512];
  };

  static PerlinNoiseData p;
  static inline float smooth(float t) { return t * t * t * (t * (t * 6.f - 15.f) + 10.f); }
  static inline float lerp  (float t, float a, float b) { return a + t * (b - a); }
  static inline float grad(int hash, float x, float y, float z) {
    const int h   = hash & 15;
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
    const int  A = p[X] + Y;
    const int  B = p[X + 1] + Y;
    const int AA = p[A] + Z;
    const int BA = p[B] + Z;
    const int BB = p[B + 1] + Z;
    const int AB = p[A + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                   grad(p[BA], x - 1, y, z)),
                           lerp(u, grad(p[AB], x, y - 1, z),
                                   grad(p[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                   grad(p[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                   grad(p[BB + 1], x - 1, y - 1, z - 1))));
  }
};
PerlinNoise::PerlinNoiseData PerlinNoise::p;

OSPGeometry PlaneGeometry(const vec4f& color, const AffineSpace3f& M)
{
  const vec3f normal = xfmNormal(M, vec3f(0.f, 1.f, 0.f));
  std::vector<vec3f> positions {
    xfmPoint(M, vec3f(-1.f, 0.f, -1.f)),
    xfmPoint(M, vec3f(+1.f, 0.f, -1.f)),
    xfmPoint(M, vec3f(+1.f, 0.f, +1.f)),
    xfmPoint(M, vec3f(-1.f, 0.f, +1.f)),
  };
  std::vector<vec3f> normals { normal, normal, normal, normal };
  std::vector<vec4f> colors  { color, color, color, color };
  std::vector<vec4i> indices { vec4i(0, 1, 2, 3) };
  //std::vector<vec3i> indices { vec3i(0, 1, 2) };

  OSPData positionData = ospNewData(positions.size(), OSP_VEC3F, positions.data());
  OSPData normalData   = ospNewData(normals.size(),   OSP_VEC3F, normals.data());
  OSPData colorData    = ospNewData(colors.size(),    OSP_VEC4F, colors.data());
  OSPData indexData = ospNewData(indices.size(), OSP_VEC4UI, indices.data());

  OSPGeometry ospGeometry = ospNewGeometry("quads");

  ospSetObject(ospGeometry, "vertex.position", positionData);
  ospSetObject(ospGeometry, "vertex.normal",   normalData);
  ospSetObject(ospGeometry, "vertex.color",    colorData);
  ospSetObject(ospGeometry, "index",           indexData);

  ospCommit(ospGeometry);

  return ospGeometry;
}
OSPGeometry PlaneGeometry(const vec4f& color) { return PlaneGeometry(color, AffineSpace3f(one)); }
OSPGeometry PlaneGeometry(const AffineSpace3f& M) { return PlaneGeometry(vec4f(0.8f, 0.8f, 0.8f, 1.f), M); }
OSPGeometry PlaneGeometry() { return PlaneGeometry(vec4f(0.8f, 0.8f, 0.8f, 1.f), AffineSpace3f(one)); }

OSPGeometry BoxGeometry(const box3f& box)
{
  auto ospGeometry = ospNewGeometry("boxes");
  auto ospData = ospNewData(1, OSP_BOX3F, &box);
  ospSetObject(ospGeometry, "box", ospData);
  ospRelease(ospData);
  ospCommit(ospGeometry);
  return ospGeometry;
}

OSPVolumetricModel CreateProceduralVolumetricModel(
  std::function<bool(vec3f p)> D,
  std::vector<vec3f> colors,
  std::vector<float> opacities,
  float densityScale,
  float anisotropy)
{
    vec3l dims{256, 256, 256}; // should be at least 2
    const float spacing = 3.f/(reduce_max(dims)-1);
    OSPVolume volume = ospNewVolume("shared_structured_volume");

    // generate volume data
    auto numVoxels = dims.product();
    std::vector<float> voxels(numVoxels, 0);
    tasking::parallel_for(dims.z, [&](int64_t z) {
      for (int y = 0; y < dims.y; ++y) {
        for (int x = 0; x < dims.x; ++x) {
          vec3f p = vec3f(x+0.5f, y+0.5f, z+0.5f)/dims;
          if (D(p)) voxels[dims.x * dims.y * z + dims.x * y + x] = 0.5f + 0.5f * PerlinNoise::noise(p, 12);
        }
      }
    });

    // calculate voxel range
    range1f voxelRange;
    std::for_each(voxels.begin(), voxels.end(), [&](float &v) {
      if (!std::isnan(v))
        voxelRange.extend(v);
    });

    OSPData voxelData = ospNewData(numVoxels, OSP_FLOAT, voxels.data());
    ospSetObject(volume, "voxelData", voxelData);
    ospRelease(voxelData);

    ospSetInt(volume, "voxelType", OSP_FLOAT);
    ospSetVec3i(volume, "dimensions", dims.x, dims.y, dims.z);
    ospSetVec3f(volume, "gridOrigin", -1.5f, -1.5f, -1.5f);
    ospSetVec3f(volume, "gridSpacing", spacing, spacing, spacing);
    ospCommit(volume);

    OSPTransferFunction tfn = ospNewTransferFunction("piecewise_linear");
    ospSetVec2f(tfn, "valueRange", voxelRange.lower, voxelRange.upper);
    OSPData tfColorData = ospNewData(colors.size(), OSP_VEC3F, colors.data());
    ospSetObject(tfn, "color", tfColorData);
    ospRelease(tfColorData);
    OSPData tfOpacityData = ospNewData(opacities.size(), OSP_FLOAT, opacities.data());
    ospSetObject(tfn, "opacity", tfOpacityData);
    ospRelease(tfOpacityData);
    ospCommit(tfn);
    auto volumeModel = ospNewVolumetricModel(volume);
    ospSetFloat(volumeModel, "densityScale", densityScale);
    ospSetFloat(volumeModel, "anisotropy", anisotropy);
    ospSetObject(volumeModel, "transferFunction", tfn);
    ospCommit(volumeModel);
    ospRelease(tfn);
    ospRelease(volume);
    return volumeModel;
}

OSPGeometricModel CreateGeometricModel(OSPGeometry geo, const vec3f& kd)
{
    OSPGeometricModel geometricModel = ospNewGeometricModel(geo);
    OSPMaterial objMaterial = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
    ospSetVec3f(objMaterial, "Kd", kd.x, kd.y, kd.z);
    ospCommit(objMaterial);
    ospSetObject(geometricModel, "material", objMaterial);
    ospRelease(objMaterial);
    return geometricModel;
}

OSPInstance CreateInstance(OSPGroup group, const AffineSpace3f& xfm)
{
  OSPInstance instance = ospNewInstance(group);
  ospSetParam(instance, "xfm", OSP_AFFINE3F, &xfm.l.vx.x);
  return instance;
}

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  float densityScale = 10.0f;
  float anisotropy = 0.0f;

  auto turbulence = [](const vec3f& p, float base_freqency, int octaves)
  {
    float value = 0.f;
    float scale = 1.f;
    for (int o = 0; o < octaves; ++o)
    {
      value += PerlinNoise::noise(scale*p, base_freqency)/scale;
      scale *= 2.f;
    }
    return value;
  };

  auto torus = [](vec3f X, float R, float r) -> bool {
    const float tmp = sqrtf(X.x * X.x + X.z * X.z) - R;
    return tmp*tmp + X.y*X.y - r*r < 0.f;
  };

  std::vector<OSPVolumetricModel> volumetricModels;
  volumetricModels.emplace_back(CreateProceduralVolumetricModel(
    [&](vec3f p) {
      vec3f X = 2.f * p - vec3f(1.f);
      return length((1.4f + 0.4 * turbulence(p, 12.f, 12)) * X) < 1.f;
    },
    {vec3f(0.f, 0.0f, 0.f), vec3f(1.f, 0.f, 0.f), vec3f(0.f, 1.f, 1.f), vec3f(1.f, 1.f, 1.f)},
    {0.f, 0.33f, 0.66f, 1.f},
    densityScale,
    anisotropy));
  volumetricModels.emplace_back(CreateProceduralVolumetricModel(
    [&](vec3f p) {
      vec3f X = 2.f * p - vec3f(1.f);
      return torus((1.4f + 0.4 * turbulence(p, 12.f, 12)) * X, 0.75f, 0.175f);
    },
    {vec3f(0.0, 0.0, 0.0), vec3f(1.0, 0.65, 0.0), vec3f(0.12, 0.6, 1.0), vec3f(1.0, 1.0, 1.0)},
    {0.f, 0.33f, 0.66f, 1.f},
    densityScale,
    anisotropy));

  for (auto volumetricModel : volumetricModels) {
    ospCommit(volumetricModel);
  }

  // create geometries
  auto box1 = BoxGeometry(box3f(vec3f(-1.5f, -1.f, -0.75f), vec3f(-0.5f, 0.f, 0.25f)));
  auto box2 = BoxGeometry(box3f(vec3f(0.0f, -1.5f, 0.f), vec3f(2.f, 1.5f, 2.f)));
  auto plane = PlaneGeometry(vec4f(vec3f(1.0f), 1.f), AffineSpace3f::translate(vec3f(0.f, -2.5f, 0.f)) * AffineSpace3f::scale(vec3f(10.f, 1.f, 10.f)));
  std::vector<OSPGeometricModel> geometricModels;
  geometricModels.emplace_back(CreateGeometricModel(box1, vec3f(0.2f, 0.2f, 0.2f)));
  geometricModels.emplace_back(CreateGeometricModel(box2, vec3f(0.2f, 0.2f, 0.2f)));
  geometricModels.emplace_back(CreateGeometricModel(plane, vec3f(0.2f, 0.2f, 0.2f)));
  ospRelease(box1);
  ospRelease(box2);
  ospRelease(plane);

  for (auto geometricModel : geometricModels) {
    ospCommit(geometricModel);
  }

  OSPWorld world = ospNewWorld();
  OSPGroup group_geometry = ospNewGroup();

  std::vector<OSPGroup> volumetricGroups;
  for (size_t i = 0; i < volumetricModels.size(); ++i)
    volumetricGroups.emplace_back(ospNewGroup());

  std::vector<OSPInstance> instances;
  instances.emplace_back(CreateInstance(group_geometry, one));
  instances.emplace_back(CreateInstance(volumetricGroups[0], AffineSpace3f::translate(vec3f(0.f, -0.5f, 0.f)) * AffineSpace3f::scale(vec3f(1.25f))));
  instances.emplace_back(CreateInstance(volumetricGroups[1], AffineSpace3f::translate(vec3f(0.f, -0.5f, 0.f)) * AffineSpace3f::scale(vec3f(2.5))));
  for (auto instance : instances)
    ospCommit(instance);

  OSPData instance_data =
      ospNewData(instances.size(), OSP_INSTANCE, instances.data());
  ospSetObject(world, "instance", instance_data);
  ospRelease(instance_data);

  // create OSPRay renderer
  int maxDepth = 32;
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());
  ospSetInt(renderer, "maxDepth", maxDepth);
  ospSetInt(renderer, "rouletteDepth", 32);
  ospSetFloat(renderer, "minContribution", 0.f);

  std::vector<OSPLight> light_handles;
  OSPLight quadLight = ospNewLight("quad");
  ospSetVec3f(quadLight, "position", -4.0f, 3.0f, 1.0f);
  ospSetVec3f(quadLight, "edge1", 0.f, 0.0f, -1.0f);
  ospSetVec3f(quadLight, "edge2", 1.0f, 0.5, 0.0f);
  ospSetFloat(quadLight, "intensity", 50.0f);
  ospSetVec3f(quadLight, "color", 2.6f, 2.5f, 2.3f);
  ospCommit(quadLight);

  OSPLight ambientLight = ospNewLight("ambient");
  ospSetFloat(ambientLight, "intensity", 0.4f);
  ospSetVec3f(ambientLight, "color", 1.f, 1.f, 1.f);
  ospCommit(ambientLight);

  bool showVolume = true;
  bool showSphere = true;
  bool showTorus  = false;
  bool showGeometry = false;
  bool enableQuadLight = true;
  bool enableAmbientLight = false;
  auto updateScene = [&]()
  {
    ospRemoveParam(group_geometry, "geometry");
    for (auto group : volumetricGroups)
      ospRemoveParam(group, "volume");

    if (showVolume) {
      for (size_t i = 0; i < volumetricGroups.size(); ++i)
      {
        if ((!showTorus && i == 1) || (!showSphere && i == 0))
          continue;

        OSPData volumes =
            ospNewData(1, OSP_VOLUMETRIC_MODEL, &volumetricModels[i]);
        ospSetObject(volumetricGroups[i], "volume", volumes);
        ospRelease(volumes);
      }
    }

    if (showGeometry) {
      OSPData geometries = ospNewData(
          geometricModels.size(), OSP_GEOMETRIC_MODEL, geometricModels.data());
      ospSetObject(group_geometry, "geometry", geometries);
      ospRelease(geometries);
    }

    light_handles.clear();
    if (enableQuadLight)
      light_handles.push_back(quadLight);
    if (enableAmbientLight)
    light_handles.push_back(ambientLight);

    OSPData lights = ospNewData(light_handles.size(), OSP_LIGHT, light_handles.data(), 0);
    ospSetObject(world, "light", lights);
    ospCommit(lights);
    ospRelease(lights);
  };

  updateScene();
  ospCommit(group_geometry);
  for (auto group : volumetricGroups)
    ospCommit(group);
  ospCommit(world);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 768},
                           box3f(vec3f(-2.2f, -1.2f, -3.2f), vec3f(1.2f, 1.2f, 1.2f)),
                           world,
                           renderer));

  // ImGui //

  glfwOSPRayWindow->registerImGuiCallback([&]() {

    bool updateWorld = false;
    bool commitWorld = false;

    if (ImGui::SliderInt("maxDepth", &maxDepth, 0, 128)) {
      commitWorld = true;
      ospSetInt(renderer, "maxDepth", maxDepth);
      glfwOSPRayWindow->addObjectToCommit(renderer);
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
      for (auto vModel : volumetricModels)
      {
        ospSetFloat(vModel, "densityScale", densityScale);
        glfwOSPRayWindow->addObjectToCommit(vModel);
      }
    }

    if (ImGui::SliderFloat("anisotropy", &anisotropy, -1.f, 1.f)) {
      commitWorld = true;
      for (auto vModel : volumetricModels)
      {
        ospSetFloat(vModel, "anisotropy", anisotropy);
        glfwOSPRayWindow->addObjectToCommit(vModel);
      }
    }

    if (updateWorld)
      updateScene();

    if (commitWorld) {
      glfwOSPRayWindow->addObjectToCommit(group_geometry);
      for (auto group : volumetricGroups)
        glfwOSPRayWindow->addObjectToCommit(group);
      glfwOSPRayWindow->addObjectToCommit(world);
      glfwOSPRayWindow->addObjectToCommit(renderer);
    }
  });

  glfwOSPRayWindow->getArcballCamera()->setRotation(quaternionf(-0.91f, 0.34f, 0.22f, -0.08f));
  glfwOSPRayWindow->updateCamera();
  glfwOSPRayWindow->commitCamera();

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  for (auto volumetricModel : volumetricModels)
    ospRelease(volumetricModel);
  for(auto geometricModel : geometricModels)
    ospRelease(geometricModel);
  ospRelease(group_geometry);
  for (auto group : volumetricGroups)
    ospRelease(group);
  for (auto instance : instances)
    ospRelease(instance);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
