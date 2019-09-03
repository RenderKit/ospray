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
  vec3l dims{512, 512, 512}; // should be at least 2
  const float spacing = 3.f/(reduce_max(dims)-1);
  OSPVolume volume = ospNewVolume("shared_structured_volume");

  // generate volume data
  auto numVoxels = dims.product();
  std::vector<float> voxels(numVoxels);
  tasking::parallel_for(dims.z, [&](int64_t z) {
    for (int y = 0; y < dims.y; ++y) {
      for (int x = 0; x < dims.x; ++x) {
        //const float X = 2.f*((float)x)/dims.x - 1.f;
        //const float Y = 2.f*((float)y)/dims.y - 1.f;
        //const float Z = 2.f*((float)z)/dims.z - 1.f;
        //float d = std::sqrt(X * X + Y * Y + Z * Z);
        //voxels[dims.x * dims.y * z + dims.x * y + x] = d < 1.f ? 1.0f : 0.f;
        voxels[dims.x * dims.y * z + dims.x * y + x] = 1.f;
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

  auto volumeModel = ospNewVolumetricModel(volume);
  ospSetObject(volumeModel, "transferFunction", tfn);
  ospSetFloat(volumeModel, "samplingRate", 0.5f);
  ospSetVec3f(volumeModel, "albedo", 1.f, 1.f, 1.f);

  OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
  ospSetFloat(volumetricMaterial, "meanCosine", 0.f);
  ospSetVec3f(volumetricMaterial, "albedo", 1.f, 1.f, 1.f);
  ospCommit(volumetricMaterial);
  ospSetObject(volumeModel, "material", volumetricMaterial);
  ospRelease(volumetricMaterial);

  ospCommit(volumeModel);
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
    auto geometry = BoxGeometry(box3f(vec3f(0.0f, 0.f, 0.f), vec3f(2.f, 2.f, 2.f)));
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
  OSPGroup group = ospNewGroup();

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);

  OSPData instances = ospNewData(1, OSP_OBJECT, &instance);
  ospSetData(world, "instance", instances);
  ospRelease(instances);

  bool showVolume = true;
  bool showGeometry = true;
  auto updateScene = [&]() {

    ospRemoveParam(group, "geometry");
    ospRemoveParam(group, "volume");

    if (showVolume) {
      OSPData volumes = ospNewData(1, OSP_OBJECT, &volumeModel);
      ospSetObject(group, "volume", volumes);
      ospRelease(volumes);
    }
    
    if (showGeometry) {
      OSPData geometries = ospNewData(geometricModels.size(), OSP_OBJECT, geometricModels.data());
      ospSetObject(group, "geometry", geometries);
      ospRelease(geometries);
    }
  };

  updateScene();
  ospCommit(group);
  ospCommit(world);

  // create OSPRay renderer
  int maxDepth = 1024;
  int rouletteDepth = 32;
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());
  ospSetInt(renderer, "maxDepth", maxDepth);
  ospSetInt(renderer, "rouletteDepth", rouletteDepth);
  ospSetFloat(renderer, "minContribution", 0.f);

  std::vector<OSPLight> light_handles;
  //{
  //  OSPLight light = ospNewLight("quad");
  //  ospSetVec3f(light, "position", -4.0f, 3.0f, 1.0f);
  //  ospSetVec3f(light, "edge1", 0.f, 0.0f, -0.5f);
  //  ospSetVec3f(light, "edge2", 0.5f, 0.25f, 0.0f);
  //  ospSetFloat(light, "intensity", 25.0f);
  //  ospSetVec3f(light, "color", 2.6f, 2.5f, 2.3f);
  //  ospCommit(light);
  //  light_handles.push_back(light);
  //}
  {
    OSPLight light = ospNewLight("ambient");
    //ospSetFloat(light, "intensity", 3.0f);
    //ospSetVec3f(light, "color", 0.03f, 0.07, 0.23);
    ospSetFloat(light, "intensity", 0.8f);
    ospSetVec3f(light, "color", 1.f, 1.f, 1.f);
    ospCommit(light);
    light_handles.push_back(light);
  }

  OSPData lights = ospNewData(light_handles.size(), OSP_LIGHT, light_handles.data(), 0);
  ospCommit(lights);
  ospSetData(renderer, "light", lights);
  ospRelease(lights);

  ospCommit(renderer);

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

    commitWorld = updateWorld;

    static vec3f albedo(1.0f);
    if (ImGui::ColorEdit3("albedo", (float*)&albedo.x, 
      ImGuiColorEditFlags_NoAlpha | 
      ImGuiColorEditFlags_HSV | 
      ImGuiColorEditFlags_Float | 
      ImGuiColorEditFlags_PickerHueWheel))
    {
      commitWorld = true;
      ospSetVec3fv(volumeModel, "albedo", (float*)&albedo.x);
      glfwOSPRayWindow->addObjectToCommit(volumeModel);
    }
    
    static float sigma_t = 1.0f;
    if (ImGui::SliderFloat("sigma_t", &sigma_t, 0.f, 10.f)) {
      commitWorld = true;
      ospSetFloat(volumeModel, "sigma_t", sigma_t);
      glfwOSPRayWindow->addObjectToCommit(volumeModel);
    }
    
    static float meanCosine = 0.0f;
    if (ImGui::SliderFloat("meanCosine", &meanCosine, -1.f, 1.f)) {
      commitWorld = true;
      OSPMaterial volumetricMaterial = ospNewMaterial(renderer_type.c_str(), "VolumetricMaterial");
      ospSetFloat(volumetricMaterial, "meanCosine", meanCosine);
      ospSetVec3fv(volumetricMaterial, "albedo", (float*)&albedo.x);
      ospCommit(volumetricMaterial);
      ospSetObject(volumeModel, "material", volumetricMaterial);
      ospRelease(volumetricMaterial);
      glfwOSPRayWindow->addObjectToCommit(volumeModel);
    }

    if (updateWorld)
      updateScene();

    if (commitWorld) {
      glfwOSPRayWindow->addObjectToCommit(group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
    if (updateWorld)
      updateScene();

    if (commitWorld) {
      glfwOSPRayWindow->addObjectToCommit(group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(volume);
  ospRelease(volumeModel);
  for(auto geometricModel : geometricModels)
    ospRelease(geometricModel);
  ospRelease(group);
  ospRelease(instance);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
