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

class PerlinNoise
{
  static constexpr unsigned int tableSize = 12;
  vec3f gradients[tableSize]; 
  unsigned int permutationTable[tableSize * 2];
  
  inline unsigned int hash(const int x, const int y, const int z) const
  {
    return permutationTable[permutationTable[permutationTable[x] + y] + z];
  }

  static inline float fade(const float t)
  {
    return t * t * t * (t * (t * 6 - 15) + 10);
  }

  static inline float fadeDeriv(const float t)
  {
    return 30 * t * t * (t * (t - 2) + 1);
  }

  static float gradientDotV(uint8_t perm, float x, float y, float z)
  {
    switch (perm & 15) 
    {
      case 0:  return +x + y;
      case 1:  return -x + y; 
      case 2:  return +x - y;
      case 3:  return -x - y;
      case 4:  return +x + z;
      case 5:  return -x + z;
      case 6:  return +x - z;
      case 7:  return -x - z;
      case 8:  return +y + z;
      case 9:  return -y + z;
      case 10: return +y - z;
      case 11: return -y - z;
      case 12: return +y + x;
      case 13: return -x + y;
      case 14: return -y + z;
      case 15: return -y - z;
    }
    return 0;
  }

 public:
  PerlinNoise(const unsigned seed = 19937)
  {
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> distribution;
    auto randf = std::bind(distribution, generator);

    for (unsigned i = 0; i < tableSize; ++i) 
    {
      float theta = acos(2 * randf() - 1);
      float phi = 2 * randf() * M_PI;
      float x = cos(phi) * sin(theta);
      float y = sin(phi) * sin(theta);
      float z = cos(theta);
      gradients[i] = vec3f(x, y, z);
      permutationTable[i] = i;
    }

    std::uniform_int_distribution<unsigned> distributionInt;
    auto randi = std::bind(distributionInt, generator);

    // create permutation table
    for (unsigned i = 0; i < tableSize; ++i)
      std::swap(permutationTable[i], permutationTable[randi() % tableSize]);

    // extend the permutation table in the index range [256:512]
    for (unsigned i = 0; i < tableSize; ++i) {
      permutationTable[tableSize + i] = permutationTable[i];
    }
  }

  vec3f deriv(vec3f p) const
  {
    p.x = fmodf(p.x * tableSize, tableSize);
    p.y = fmodf(p.y * tableSize, tableSize);
    p.z = fmodf(p.z * tableSize, tableSize);

    int xi0 = ((int)std::floor(p.x)) % tableSize;
    int yi0 = ((int)std::floor(p.y)) % tableSize;
    int zi0 = ((int)std::floor(p.z)) % tableSize;

    int xi1 = (xi0 + 1) % tableSize;
    int yi1 = (yi0 + 1) % tableSize;
    int zi1 = (zi0 + 1) % tableSize;

    float tx = p.x - ((int)std::floor(p.x));
    float ty = p.y - ((int)std::floor(p.y));
    float tz = p.z - ((int)std::floor(p.z));

    float u = fade(tx);
    float v = fade(ty);
    float w = fade(tz);

    // generate vectors going from the grid points to p
    float x0 = tx, x1 = tx - 1;
    float y0 = ty, y1 = ty - 1;
    float z0 = tz, z1 = tz - 1;

    float a = gradientDotV(hash(xi0, yi0, zi0), x0, y0, z0);
    float b = gradientDotV(hash(xi1, yi0, zi0), x1, y0, z0);
    float c = gradientDotV(hash(xi0, yi1, zi0), x0, y1, z0);
    float d = gradientDotV(hash(xi1, yi1, zi0), x1, y1, z0);
    float e = gradientDotV(hash(xi0, yi0, zi1), x0, y0, z1);
    float f = gradientDotV(hash(xi1, yi0, zi1), x1, y0, z1);
    float g = gradientDotV(hash(xi0, yi1, zi1), x0, y1, z1);
    float h = gradientDotV(hash(xi1, yi1, zi1), x1, y1, z1);

    float du = fadeDeriv(tx);
    float dv = fadeDeriv(ty);
    float dw = fadeDeriv(tz);

    float k1 = (b - a);
    float k2 = (c - a);
    float k3 = (e - a);
    float k4 = (a + d - b - c);
    float k5 = (a + f - b - e);
    float k6 = (a + g - c - e);
    float k7 = (b + c + e + h - a - d - f - g);

    vec3f derivs;
    derivs.x = du * (k1 + k4 * v + k5 * w + k7 * v * w);
    derivs.y = dv * (k2 + k4 * u + k6 * w + k7 * v * w);
    derivs.z = dw * (k3 + k5 * u + k6 * v + k7 * v * w);

    return derivs;
  } 
  
  float noise(vec3f p) const
  {
    p.x = fmodf(p.x * tableSize, tableSize);
    p.y = fmodf(p.y * tableSize, tableSize);
    p.z = fmodf(p.z * tableSize, tableSize);

    int xi0 = ((int)std::floor(p.x)) % tableSize;
    int yi0 = ((int)std::floor(p.y)) % tableSize;
    int zi0 = ((int)std::floor(p.z)) % tableSize;

    int xi1 = (xi0 + 1) % tableSize;
    int yi1 = (yi0 + 1) % tableSize;
    int zi1 = (zi0 + 1) % tableSize;

    float tx = p.x - ((int)std::floor(p.x));
    float ty = p.y - ((int)std::floor(p.y));
    float tz = p.z - ((int)std::floor(p.z));

    float u = fade(tx);
    float v = fade(ty);
    float w = fade(tz);

    // generate vectors going from the grid points to p
    float x0 = tx, x1 = tx - 1;
    float y0 = ty, y1 = ty - 1;
    float z0 = tz, z1 = tz - 1;

    float a = gradientDotV(hash(xi0, yi0, zi0), x0, y0, z0);
    float b = gradientDotV(hash(xi1, yi0, zi0), x1, y0, z0);
    float c = gradientDotV(hash(xi0, yi1, zi0), x0, y1, z0);
    float d = gradientDotV(hash(xi1, yi1, zi0), x1, y1, z0);
    float e = gradientDotV(hash(xi0, yi0, zi1), x0, y0, z1);
    float f = gradientDotV(hash(xi1, yi0, zi1), x1, y0, z1);
    float g = gradientDotV(hash(xi0, yi1, zi1), x0, y1, z1);
    float h = gradientDotV(hash(xi1, yi1, zi1), x1, y1, z1);

    float k0 = a;
    float k1 = (b - a);
    float k2 = (c - a);
    float k3 = (e - a);
    float k4 = (a + d - b - c);
    float k5 = (a + f - b - e);
    float k6 = (a + g - c - e);
    float k7 = (b + c + e + h - a - d - f - g);

    return k0 + k1 * u + k2 * v + k3 * w + k4 * u * v + k5 * u * w +
           k6 * v * w + k7 * u * v * w; 
  } 
};

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
  OSPData indexData    = ospNewData(indices.size(),   OSP_VEC4I, indices.data());

  OSPGeometry ospGeometry = ospNewGeometry("quads");

  ospSetData(ospGeometry, "vertex.position", positionData);
  ospSetData(ospGeometry, "vertex.normal",   normalData);
  ospSetData(ospGeometry, "vertex.color",    colorData);
  ospSetData(ospGeometry, "index",           indexData);

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
  ospSetData(ospGeometry, "box", ospData);
  ospRelease(ospData);
  ospCommit(ospGeometry);
  return ospGeometry;
}

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // Create volume
  vec3l dims{256, 256, 256}; // should be at least 2
  const float spacing = 3.f/(reduce_max(dims)-1);
  OSPVolume volume = ospNewVolume("shared_structured_volume");

  PerlinNoise pnoise;

  //auto turbulence_noise = [&pnoise](int octaves, vec3f p)
  //{
  //  float val = 0;
  //  for (int o = 0; o < octaves; ++o)
  //  {
  //    float scale = (float)(1 << o);
  //    val += pnoise.noise(scale*p)/scale;
  //  }
  //  return val;
  //};
  
  auto turbulence_deriv = [&pnoise](int octaves, vec3f p)
  {
    vec3f deriv(0.f);
    for (int o = 0; o < octaves; ++o)
    {
      float scale = (float)(1 << o);
      deriv += pnoise.deriv(scale*p)/scale;
    }
    return deriv;
  };

  // generate volume data
  auto numVoxels = dims.product();
  std::vector<float> voxels(numVoxels);
  tasking::parallel_for(dims.z, [&](int64_t z) {
    for (int y = 0; y < dims.y; ++y) {
      for (int x = 0; x < dims.x; ++x) {
        int64_t offset = dims.x * dims.y * z + dims.x * y + x;
        vec3f p(((float)x)/dims.x, ((float)y)/dims.y, ((float)z)/dims.z);
        #if 0
        const float X = 2.f*((float)x)/dims.x - 1.f;
        const float Y = 2.f*((float)y)/dims.y - 1.f;
        const float Z = 2.f*((float)z)/dims.z - 1.f;
        float d = std::sqrt(X * X + Y * Y + Z * Z);
        if (d < 1.f) {
          voxels[offset] = d * turbulence_noise(10, p) + (1.f - d) * 1.f;
          voxels[offset] = d * turbulence_noise(10, p) + (1.f - d) * 1.f;
        } else {
          voxels[offset] = 0.f;
        }
        #else
        vec3f deriv = 0.05f * turbulence_deriv(10, p);
        const float X = (2.f*((float)x)/dims.x) - 1.f + deriv.x;
        const float Y = (2.f*((float)y)/dims.y) - 1.f + deriv.y;
        const float Z = (2.f*((float)z)/dims.z) - 1.f + deriv.z;
        float d = std::sqrt(X * X + Y * Y + Z * Z);
        if (d < 0.9f) {
          voxels[offset] = 1.f;
        } else {
          voxels[offset] = 0.f;
        }
        #endif
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
  ospSetVec3f(volume, "gridOrigin", -1.5f, -2.0f, -1.5f);
  ospSetVec3f(volume, "gridSpacing", spacing, spacing, spacing);
  ospCommit(volume);

  OSPTransferFunction tfn =
      ospNewTransferFunction("piecewise_linear");
  ospSetVec2f(tfn, "valueRange", voxelRange.lower, voxelRange.upper);
  float colors[]      = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  float opacites[]    = {0.f, 1.0f};
  OSPData tfColorData = ospNewData(2, OSP_VEC3F, colors);
  ospSetData(tfn, "color", tfColorData);
  ospRelease(tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(tfn, "opacity", tfOpacityData);
  ospRelease(tfOpacityData);
  ospCommit(tfn);

  std::vector<OSPVolumetricModel> volumetricModels;
  {
    auto volumeModel = ospNewVolumetricModel(volume);
    ospSetVec3f(volumeModel, "albedo", 0.2f, 0.8f, 0.2f);
    ospSetFloat(volumeModel, "sigma_t", 4.f);
    ospSetObject(volumeModel, "transferFunction", tfn);

    OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
    ospSetFloat(volumetricMaterial, "meanCosine", 0.f);
    ospSetVec3f(volumetricMaterial, "albedo", 0.2f, 0.8f, 0.2f);
    ospCommit(volumetricMaterial);
    ospSetObject(volumeModel, "material", volumetricMaterial);
    ospRelease(volumetricMaterial);
    ospCommit(volumeModel);
    volumetricModels.push_back(volumeModel);
  }
  {
    auto volumeModel = ospNewVolumetricModel(volume);
    ospSetVec3f(volumeModel, "albedo", 0.8f, 0.2f, 0.8f);
    ospSetFloat(volumeModel, "sigma_t", 5.f);
    ospSetObject(volumeModel, "transferFunction", tfn);

    OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
    ospSetFloat(volumetricMaterial, "meanCosine", 0.f);
    ospSetVec3f(volumetricMaterial, "albedo", 0.8f, 0.2f, 0.8f);
    ospCommit(volumetricMaterial);
    ospSetObject(volumeModel, "material", volumetricMaterial);
    ospRelease(volumetricMaterial);
    ospCommit(volumeModel);
    volumetricModels.push_back(volumeModel);
  }
  {
    auto volumeModel = ospNewVolumetricModel(volume);
    ospSetVec3f(volumeModel, "albedo", 0.2f, 0.8f, 0.8f);
    ospSetFloat(volumeModel, "sigma_t", 3.f);
    ospSetObject(volumeModel, "transferFunction", tfn);

    OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
    ospSetFloat(volumetricMaterial, "meanCosine", 0.f);
    ospSetVec3f(volumetricMaterial, "albedo", 0.2f, 0.8f, 0.8f);
    ospCommit(volumetricMaterial);
    ospSetObject(volumeModel, "material", volumetricMaterial);
    ospRelease(volumetricMaterial);
    ospCommit(volumeModel);
    volumetricModels.push_back(volumeModel);
  }
  ospRelease(tfn);

  // create geometries
  std::vector<OSPGeometricModel> geometricModels;
  if (1) {
    auto geometry = BoxGeometry(box3f(vec3f(-1.5f, -1.f, -1.f), vec3f(-0.5f, 0.f, 0.f)));
    geometricModels.emplace_back(ospNewGeometricModel(geometry));
    ospRelease(geometry);
    vec4f color(1.0f);
    auto colorData = ospNewData(1, OSP_VEC4F, &color);
    ospSetData(geometricModels.back(), "prim.color", colorData);
    ospRelease(colorData);
    OSPMaterial objMaterial = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
    ospSetVec3f(objMaterial, "Kd", 0.2f, 0.2f, 0.8f);
    ospCommit(objMaterial);
    ospSetObject(geometricModels.back(), "material", objMaterial);
    ospRelease(objMaterial);
  }
  
  if (1) {
    auto geometry = BoxGeometry(box3f(vec3f(0.0f, -1.5f, 0.f), vec3f(2.f, 1.5f, 2.f)));
    geometricModels.emplace_back(ospNewGeometricModel(geometry));
    ospRelease(geometry);
    vec4f color(1.0f);
    auto colorData = ospNewData(1, OSP_VEC4F, &color);
    ospSetData(geometricModels.back(), "prim.color", colorData);
    ospRelease(colorData);
    OSPMaterial objMaterial = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
    ospSetVec3f(objMaterial, "Kd", 0.8f, 0.2f, 0.2f);
    ospCommit(objMaterial);
    ospSetObject(geometricModels.back(), "material", objMaterial);
    ospRelease(objMaterial);
  }

  {
    auto geometry = PlaneGeometry(
      vec4f(vec3f(1.0f), 1.f),
      AffineSpace3f::translate(vec3f(0.f, -2.5f, 0.f)) * 
      AffineSpace3f::scale(vec3f(10.f, 1.f, 10.f)));
    geometricModels.emplace_back(ospNewGeometricModel(geometry));
    ospRelease(geometry);
    OSPMaterial objMaterial = ospNewMaterial("pathtracer", "OBJMaterial");
    ospSetVec3f(objMaterial, "Kd", 0.8f, 0.8f, 0.8f);
    ospSetObject(geometricModels.back(), "material", objMaterial);
    ospCommit(objMaterial);
    ospRelease(objMaterial);
  }

  for (auto geometricModel : geometricModels) {
    ospCommit(geometricModel);
  }

  OSPWorld world = ospNewWorld();
  OSPGroup group_geometry = ospNewGroup();

  std::vector<OSPGroup> volumetricGroups;
  for (size_t i = 0; i < volumetricModels.size(); ++i)
    volumetricGroups.emplace_back(ospNewGroup());

  OSPInstance instance_geometry = ospNewInstance(group_geometry);
  ospCommit(instance_geometry);
  
  OSPInstance instance_volume_0 = ospNewInstance(volumetricGroups[0]);
  //{
  //  AffineSpace3f xfm = AffineSpace3f(LinearSpace3f::rotate(vec3f(0.f, 1.f, 0.f), M_PI/4.f));
  //  ospSetAffine3fv(instance_volume_0, "xfm", &xfm.l.vx.x);
  //}
  ospCommit(instance_volume_0);
  
  OSPInstance instance_volume_1 = ospNewInstance(volumetricGroups[1]);
  {
    AffineSpace3f xfm = AffineSpace3f::translate(vec3f(1.f, -1.f, -1.f));
    ospSetAffine3fv(instance_volume_1, "xfm", &xfm.l.vx.x);
  }
  ospCommit(instance_volume_1);

  OSPInstance instance_volume_2 = ospNewInstance(volumetricGroups[2]);
  {
    AffineSpace3f xfm = AffineSpace3f::translate(vec3f(-1.f, -2.f, 1.f));
    ospSetAffine3fv(instance_volume_2, "xfm", &xfm.l.vx.x);
  }
  ospCommit(instance_volume_2);

  std::vector<OSPInstance> instances = { instance_geometry, instance_volume_0, instance_volume_1, instance_volume_2 };
  OSPData instance_data = ospNewData(instances.size(), OSP_OBJECT, instances.data());
  ospSetData(world, "instance", instance_data);
  ospRelease(instance_data);
  
  // create OSPRay renderer
  int maxDepth = 1024;
  int rouletteDepth = 32;
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());
  ospSetInt(renderer, "maxDepth", maxDepth);
  ospSetInt(renderer, "rouletteDepth", rouletteDepth);
  ospSetFloat(renderer, "minContribution", 0.f);

  std::vector<OSPLight> light_handles;
  OSPLight quadLight = ospNewLight("quad");
  ospSetVec3f(quadLight, "position", -4.0f, 3.0f, 1.0f);
  ospSetVec3f(quadLight, "edge1", 0.f, 0.0f, -0.5f);
  ospSetVec3f(quadLight, "edge2", 0.5f, 0.25f, 0.0f);
  ospSetFloat(quadLight, "intensity", 50.0f);
  ospSetVec3f(quadLight, "color", 2.6f, 2.5f, 2.3f);
  ospCommit(quadLight);

  OSPLight ambientLight = ospNewLight("ambient");
  //ospSetFloat(ambientLight, "intensity", 3.0f);
  //ospSetVec3f(ambientLight, "color", 0.03f, 0.07, 0.23);
  ospSetFloat(ambientLight, "intensity", 0.8f);
  ospSetVec3f(ambientLight, "color", 1.f, 1.f, 1.f);
  ospCommit(ambientLight);

  bool showVolume = true;
  bool showGeometry = true;
  bool enableQuadLight = true;
  bool enableAmbientLight = true;
  auto updateScene = [&]() {

    ospRemoveParam(group_geometry, "geometry");
    for (auto group : volumetricGroups)
      ospRemoveParam(group, "volume");

    if (showVolume) {
      for (size_t i = 0; i < volumetricGroups.size(); ++i)
      {
        OSPData volumes = ospNewData(1, OSP_OBJECT, &volumetricModels[i]);
        ospSetObject(volumetricGroups[i], "volume", volumes);
        ospRelease(volumes);
      }
    }
    
    if (showGeometry) {
      OSPData geometries = ospNewData(geometricModels.size(), OSP_OBJECT, geometricModels.data());
      ospSetObject(group_geometry, "geometry", geometries);
      ospRelease(geometries);
    }

    light_handles.clear();
    if (enableQuadLight)
      light_handles.push_back(quadLight);
    if (enableAmbientLight)
    light_handles.push_back(ambientLight);

    OSPData lights = ospNewData(light_handles.size(), OSP_LIGHT, light_handles.data(), 0);
    ospCommit(lights);
    ospSetData(renderer, "light", lights);
    ospRelease(lights);

    ospCommit(renderer);
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

    if (ImGui::SliderInt("maxDepth", &maxDepth, 0, 1024)) {
      commitWorld = true;
      ospSetInt(renderer, "maxDepth", maxDepth);
      glfwOSPRayWindow->addObjectToCommit(renderer);
    }
    
    if (ImGui::SliderInt("rouletteDepth", &rouletteDepth, 0, 1024)) {
      commitWorld = true;
      ospSetInt(renderer, "rouletteDepth", rouletteDepth);
      glfwOSPRayWindow->addObjectToCommit(renderer);
    }

    if (ImGui::Checkbox("Show Volume", &showVolume))
      updateWorld = true;
    if (ImGui::Checkbox("Show Geometry", &showGeometry))
      updateWorld = true;
    if (ImGui::Checkbox("Quad Light", &enableQuadLight))
      updateWorld = true;
    if (ImGui::Checkbox("Ambient Light", &enableAmbientLight))
      updateWorld = true;

    commitWorld = updateWorld;

    static vec3f albedo(1.0f);
    if (ImGui::ColorEdit3("albedo", (float*)&albedo.x, 
      ImGuiColorEditFlags_NoAlpha | 
      ImGuiColorEditFlags_HSV | 
      ImGuiColorEditFlags_Float | 
      ImGuiColorEditFlags_PickerHueWheel))
    {
      commitWorld = true;
      ospSetVec3fv(volumetricModels[0], "albedo", (float*)&albedo.x);
      glfwOSPRayWindow->addObjectToCommit(volumetricModels[0]);
    }
    
    static float sigma_t = 1.0f;
    if (ImGui::SliderFloat("sigma_t", &sigma_t, 0.f, 10.f)) {
      commitWorld = true;
      ospSetFloat(volumetricModels[0], "sigma_t", sigma_t);
      glfwOSPRayWindow->addObjectToCommit(volumetricModels[0]);
    }
    
    static float meanCosine = 0.0f;
    if (ImGui::SliderFloat("meanCosine", &meanCosine, -1.f, 1.f)) {
      commitWorld = true;
      OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
      ospSetFloat(volumetricMaterial, "meanCosine", meanCosine);
      ospSetVec3fv(volumetricMaterial, "albedo", (float*)&albedo.x);
      ospCommit(volumetricMaterial);
      ospSetObject(volumetricModels[0], "material", volumetricMaterial);
      ospRelease(volumetricMaterial);
      glfwOSPRayWindow->addObjectToCommit(volumetricModels[0]);
    }

    if (updateWorld)
      updateScene();

    if (commitWorld) {
      glfwOSPRayWindow->addObjectToCommit(group_geometry);
      for (auto group : volumetricGroups)
        glfwOSPRayWindow->addObjectToCommit(group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(volume);
  for (auto volumetricModel : volumetricModels)
    ospRelease(volumetricModel);
  for(auto geometricModel : geometricModels)
    ospRelease(geometricModel);
  ospRelease(group_geometry);
  for (auto group : volumetricGroups)
    ospRelease(group);
  ospRelease(instance_geometry);
  ospRelease(instance_volume_0);
  ospRelease(instance_volume_1);
  ospRelease(instance_volume_2);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
