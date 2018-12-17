// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#include "ospray_test_fixture.h"

extern OSPRayEnvironment * ospEnv;

namespace OSPRayTestScenes {

//helper functions

//creates a torus
// volumetric data: stores data of torus
// returns created ospvolume of torus
static OSPVolume CreateTorus(std::vector<float>& volumetricData, const int size)
{
  volumetricData.resize(size*size*size, 0);

  const float r = 30;
  const float R = 80;
  const int size_2 = size/2;

  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        const float X = x - size_2;
        const float Y = y - size_2;
        const float Z = z - size_2;

        const float d = (R - std::sqrt(X*X + Y*Y));
        volumetricData[size*size * x + size * y + z] = r*r - d*d - Z*Z;
      }
    }
  }

  OSPVolume torus = ospNewVolume("shared_structured_volume");
  OSPData voxelsData = ospNewData(size * size * size, OSP_FLOAT,
                                  volumetricData.data(),
                                  OSP_DATA_SHARED_BUFFER);
  ospSetData(torus, "voxelData", voxelsData);
  ospRelease(voxelsData);
  ospSet3i(torus, "dimensions", size, size, size);
  ospSetString(torus, "voxelType", "float");
  ospSet2f(torus, "voxelRange", -10000.f, 10000.f);
  ospSet3f(torus, "gridOrigin", -0.5f, -0.5f, -0.5f);
  ospSet3f(torus, "gridSpacing", 1.f / size, 1.f / size, 1.f / size);
  return torus;
}

Base::Base()
{
  const ::testing::TestCase* const testCase = ::testing::UnitTest::GetInstance()->current_test_case();
  const ::testing::TestInfo* const testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
  imgSize = ospEnv->GetImgSize();

  {
    std::string testCaseName = testCase->name();
    std::string testInfoName = testInfo->name();
    size_t pos = testCaseName.find('/');
    if (pos == std::string::npos) {
      testName = testCaseName + "_" + testInfoName;
    } else {
      std::string instantiationName = testCaseName.substr(0, pos);
      std::string className = testCaseName.substr(pos + 1);
      testName = className + "_" + instantiationName + "_" + testInfoName;
    }
    for (char& byte : testName)
      if (byte == '/')
        byte = '_';
  }

  rendererType = "scivis";
  frames = 1;
  samplesPerPixel = 50;
  SetImageTool();
}

void Base::SetImageTool()
{
  try {
    imageTool = new OSPImageTools(imgSize, GetTestName(), frameBufferFormat);
  } catch (std::bad_alloc &e) {
    FAIL() << "Failed to create image tool.\n";
  }
}

Base::~Base()
{
#if 0 // TODO: leaking lights...releasing them segfaults with gcc (?!?!?)
  ospRelease(lights);

  for (auto l : lightsList)
    ospRelease(l);
#endif

  ospRelease(framebuffer);
  ospRelease(renderer);
  ospRelease(camera);
  ospRelease(world);

  delete imageTool;
}

void Base::SetUp()
{
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());
}

void Base::AddLight(OSPLight light)
{
  lightsList.push_back(light);
}

void Base::AddGeometry(OSPGeometry new_geometry)
{
  ospAddGeometry(world, new_geometry);
  ospRelease(new_geometry);
}

void Base::AddVolume(OSPVolume new_volume)
{
  ospAddVolume(world, new_volume);
  ospRelease(new_volume);
}

void Base::PerformRenderTest()
{
  SetLights();

  ospCommit(camera);
  ospCommit(world);
  ospCommit(renderer);

  ospFrameBufferClear(framebuffer, OSP_FB_ACCUM);

  RenderFrame(OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
  uint32_t* framebuffer_data = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);

  if(ospEnv->GetDumpImg()) {
    EXPECT_EQ(imageTool->saveTestImage(framebuffer_data), OsprayStatus::Ok);
  } else {
    EXPECT_EQ(imageTool->compareImgWithBaseline(framebuffer_data), OsprayStatus::Ok);
  }

  ospUnmapFrameBuffer(framebuffer_data, framebuffer);
}

void Base::CreateEmptyScene()
{
  SetCamera();
  SetWorld();
  SetRenderer();
  SetLights();
  SetFramebuffer();
}

void Base::SetCamera()
{
  float cam_pos[] = {0.f, 0.f, 0.f};
  float cam_up [] = {0.f, 1.f, 0.f};
  float cam_view [] = {0.f, 0.f, 1.f};

  camera = ospNewCamera("perspective");

  ospSetf(camera, "aspect", imgSize.x/(float)imgSize.y);
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);
}

void Base::SetWorld()
{
  world = ospNewModel();
}

void Base::SetLights()
{
  lights = ospNewData(lightsList.size(), OSP_LIGHT, lightsList.data());
  ospSetObject(renderer, "lights", lights);
  ospRelease(lights);
  ospCommit(lights);
}

void Base::SetRenderer()
{
  renderer = ospNewRenderer(rendererType.c_str());
  ospSet1i(renderer, "aoSamples", 1);
  ospSet1f(renderer, "bgColor", 1.0f);
  ospSetObject(renderer, "model",  world);
  ospSetObject(renderer, "camera", camera);
  ospSet1i(renderer, "spp", samplesPerPixel);
}

void Base::SetFramebuffer()
{
  framebuffer = ospNewFrameBuffer(imgSize, frameBufferFormat,
                                  OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
}

OSPMaterial Base::CreateMaterial(std::string type)
{
  OSPMaterial material = ospNewMaterial2(rendererType.c_str(), type.data());
  EXPECT_TRUE(material);
  if (type == "Glass") {
    ospSetf(material, "eta", 1.5);
    ospSet3f(material, "attenuationColor", 0.f, 1.f, 1.f);
    ospSetf(material, "attenuationDistance", 5.0);
  } else if (type == "Luminous") {
    ospSetf(material, "intensity", 3.0);
  }

  ospCommit(material);

  return material;
}

void Base::RenderFrame(const uint32_t frameBufferChannels)
{
  for (int frame = 0; frame < frames; ++frame)
    ospRenderFrame(framebuffer, renderer, frameBufferChannels);
}


SingleObject::SingleObject()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  materialType = std::get<1>(params);
}

void SingleObject::SetUp()
{
  Base::SetUp();

  SetMaterial();

  OSPLight distant = ospNewLight(renderer, "distant");
  ASSERT_TRUE(distant) << "Failed to create lights";
  ospSetf(distant, "intensity", 1.0f);
  ospSet3f(distant, "direction", 1.0f, 1.0f, 1.0f);
  ospSet1f(distant, "angularDiameter", 1.0f);
  ospCommit(distant);
  AddLight(distant);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.2f);
  ospCommit(ambient);
  AddLight(ambient);
}

void SingleObject::SetMaterial()
{
  material = CreateMaterial(materialType);
}


SpherePrecision::SpherePrecision()
{
  auto params = GetParam();
  radius = std::get<0>(params);
  dist = radius * std::get<1>(params);
  move_cam = std::get<2>(params);
  rendererType = std::get<3>(params);
}

void SpherePrecision::SetUp()
{
  Base::SetUp();

  float fov = 160.0f * std::min(std::tan(radius/std::abs(dist)), 1.0f);
  float cent = move_cam ? 0.0f : dist+radius;

  ospSet3f(camera, "pos", 0.f, 0.f, move_cam ? -dist - radius : 0.0f);
  ospSet3f(camera, "dir", 0.f, 0.f, 1.f);
  ospSet3f(camera, "up", 0.f, 1.f, 0.f);
  ospSet1f(camera, "fovy", fov);

  ospSet1i(renderer, "aoSamples", 4);
  ospSet1i(renderer, "shadowsEnabled", 1);

  ospSet1i(renderer, "spp", 4);
  ospSet4f(renderer, "bgColor", 0.2f, 0.2f, 0.4f, 1.0f);
  ospSet1i(renderer, "maxDepth", 2);

  OSPGeometry sphere = ospNewGeometry("spheres");
  OSPGeometry inst_sphere = ospNewGeometry("spheres");

  float sph_center_r[]{
    -0.5f*radius, -0.3f*radius, cent,      radius,
     0.8f*radius, -0.3f*radius, cent, 0.9f*radius,
     0.8f*radius,  1.5f*radius, cent, 0.9f*radius,
     0.0f,  -10001.3f*radius, cent, 10000.f*radius,
     0.f, 0.f, 0.f, 90.f*radius
  };
  auto data = ospNewData(4, OSP_FLOAT4, sph_center_r);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  ospRelease(data);

  data = ospNewData(1, OSP_FLOAT4, sph_center_r+4*4);
  ospCommit(data);
  ospSetData(inst_sphere, "spheres", data);
  ospRelease(data);

  OSPMaterial sphereMaterial = ospNewMaterial2(rendererType.c_str(), "default");
  ospSet1f(sphereMaterial, "d", 1.0f);
  ospCommit(sphereMaterial);
  ospSetMaterial(sphere, sphereMaterial);
  ospSetMaterial(inst_sphere, sphereMaterial);
  ospRelease(sphereMaterial);

  ospSet1i(sphere, "offset_radius", 12);
  ospCommit(sphere);

  ospSet1i(inst_sphere, "offset_radius", 12);
  ospCommit(inst_sphere);

  ospAddGeometry(world, sphere);

  OSPModel inst = ospNewModel();
  ospAddGeometry(inst, inst_sphere);
  ospRelease(inst_sphere);
  osp::affine3f xf{
    0.01, 0, 0,
    0, 0.01, 0,
    0, 0, 0.01,
   -0.5f*radius, 1.6f*radius, cent
  };
  ospAddGeometry(world, ospNewInstance(inst, xf));
  ospRelease(inst);

  OSPLight distant = ospNewLight(renderer, "distant");
  ASSERT_TRUE(distant) << "Failed to create lights";
  ospSetf(distant, "intensity", 3.0f);
  ospSet3f(distant, "direction", 0.3f, -4.0f, 0.8f);
  ospSet3f(distant, "color", 1.0f, 0.5f, 0.5f);
  ospSet1f(distant, "angularDiameter", 1.0f);
  ospCommit(distant);
  AddLight(distant);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.1f);
  ospCommit(ambient);
  AddLight(ambient);
}


Box::Box()
{
  rendererType = "pathtracer";

  auto params = GetParam();
  cuboidMaterialType = std::get<0>(params);
  sphereMaterialType = std::get<1>(params);
}

void Box::SetUp()
{
  Base::SetUp();

  SetMaterials();

  float wallsVertices[] = {
    // left wall
     1.f, -1.f, 4.f,
     1.f,  1.f, 4.f,
     1.f,  1.f, 0.f,
     1.f, -1.f, 0.f,
    // right wall
    -1.f, -1.f, 4.f,
    -1.f,  1.f, 4.f,
    -1.f,  1.f, 0.f,
    -1.f, -1.f, 0.f,
    // bottom, back and top walls
    -1.f, -1.f, 0.f,
     1.f, -1.f, 0.f,
     1.f, -1.f, 4.f,
    -1.f, -1.f, 4.f,
    -1.f,  1.f, 4.f,
     1.f,  1.f, 4.f,
     1.f,  1.f, 0.f,
    -1.f,  1.f, 0.f
  };
  float wallsColors[] = {
    1.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f
  };
  int32_t wallsIndices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    10, 11, 12, 12, 13, 10,
    12, 13, 14, 14, 15, 12
  };
  OSPGeometry wallsMesh = ospNewGeometry("triangles");
  OSPData data = ospNewData(16, OSP_FLOAT3, wallsVertices);
  ospCommit(data);
  ospSetData(wallsMesh, "vertex", data);
  ospRelease(data);
  data = ospNewData(16, OSP_FLOAT4, wallsColors);
  ospCommit(data);
  ospSetData(wallsMesh, "vertex.color", data);
  ospRelease(data);
  data = ospNewData(10, OSP_INT3, wallsIndices);
  ospCommit(data);
  ospSetData(wallsMesh, "index", data);
  ospRelease(data);
  OSPMaterial wallsMaterial = GetMaterial("OBJMaterial");
  ospSetMaterial(wallsMesh, wallsMaterial);
  ospCommit(wallsMesh);
  AddGeometry(wallsMesh);

  float lightVertices[] = {
    -0.3f, 0.999f, 2.7f,
     0.3f, 0.999f, 2.7f,
     0.3f, 0.999f, 3.3f,
    -0.3f, 0.999f, 3.3f
  };
  int32_t lightIndices[] = { 0, 1, 2, 2, 3, 0 };
  OSPGeometry lightSquare = ospNewGeometry("triangles");
  data = ospNewData(4, OSP_FLOAT3, lightVertices);
  ospCommit(data);
  ospSetData(lightSquare, "vertex", data);
  ospRelease(data);
  data = ospNewData(2, OSP_INT3, lightIndices);
  ospSetData(lightSquare, "index", data);
  ospRelease(data);
  OSPMaterial lightMaterial = ospNewMaterial2(rendererType.c_str(), "Luminous");
  ospSetf(lightMaterial, "intensity", 20.f);
  ospSet3f(lightMaterial, "color", 1.f, 0.7f, 0.3f);
  ospCommit(lightMaterial);
  ospSetMaterial(lightSquare, lightMaterial);
  ospCommit(lightSquare);
  AddGeometry(lightSquare);

  float cuboidVertices[] = {
    0.7f, -0.999f, 3.3f,
    0.3f, -0.999f, 3.6f,
    0.0f, -0.999f, 3.2f,
    0.4f, -0.999f, 2.9f,
    0.4f,  0.2, 2.9f,
    0.0f,  0.2, 3.2f,
    0.3f,  0.2, 3.6f,
    0.7f,  0.2, 3.3f,
  };
  int32_t cuboidIndices[] = {
    0, 1, 2, 2, 3, 1,
    4, 5, 6, 6, 7, 4,
    0, 1, 6, 6, 7, 0,
    1, 2, 5, 5, 6, 1,
    2, 3, 4, 4, 5, 2,
    3, 0, 7, 7, 4, 3
  };
  OSPGeometry cuboid = ospNewGeometry("triangles");
  data = ospNewData(8, OSP_FLOAT3, cuboidVertices);
  ospCommit(data);
  ospSetData(cuboid, "vertex", data);
  ospRelease(data);
  data = ospNewData(12, OSP_INT3, cuboidIndices);
  ospCommit(data);
  ospSetData(cuboid, "index", data);
  ospRelease(data);
  ospSetMaterial(cuboid, cuboidMaterial);
  ospCommit(cuboid);
  AddGeometry(cuboid);

  float sphereVertex[] = { -0.3f, -0.55f, 2.5f, 0.0f};
  OSPGeometry sphere = ospNewGeometry("spheres");
  data = ospNewData(1, OSP_FLOAT4, sphereVertex);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  ospRelease(data);
  ospSet1f(sphere, "radius", 0.45f);
  ospSetMaterial(sphere, sphereMaterial);
  ospCommit(sphere);
  AddGeometry(sphere);
}

void Box::SetMaterials()
{
  cuboidMaterial = GetMaterial(cuboidMaterialType);
  sphereMaterial = GetMaterial(sphereMaterialType);
}

OSPMaterial Box::GetMaterial(std::string type)
{
  OSPMaterial newMaterial = ospNewMaterial2(rendererType.c_str(), type.data());
  // defaults for "OBJMaterial"
  if (type == "Glass") {
    ospSetf(newMaterial, "eta", 1.4);
  } else if (type == "Luminous") {
    ospSetf(newMaterial, "intensity", 0.7f);
  }
  ospCommit(newMaterial);

  return newMaterial;
}


Sierpinski::Sierpinski()
{
  auto params = GetParam();
  renderIsosurface = std::get<1>(params);
  level = std::get<2>(params);

  rendererType = std::get<0>(params);
}

void Sierpinski::SetUp()
{
  Base::SetUp();

  float cam_pos[] = {-0.5f, -1.f, 0.2f};
  float cam_up [] = {0.f, 0.f, -1.f};
  float cam_view [] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);

  int size = 1 << level;

  volumetricData.resize(size*size*size, 0);

  auto getIndex = [size](int layer, int x, int y) {
    return size*size * layer + size * x + y;
  };

  volumetricData[getIndex(0, size-1, size-1)] = 255;
  for (int layer = 1; layer < size; ++layer) {
    for (int x = size - layer - 1; x < size; ++x) {
      for (int y = size - layer - 1; y < size; ++y) {
        int counter = 0;
        if (volumetricData[getIndex(layer-1, x, y)]) { counter++; }
        if (volumetricData[getIndex(layer-1, (x+1)%size, y)]) { counter++; }
        if (volumetricData[getIndex(layer-1, x, (y+1)%size)]) { counter++; }

        volumetricData[getIndex(layer, x, y)] = (counter == 1) ? 255 : 0;
       }
    }
  }

  OSPVolume pyramid = ospNewVolume("shared_structured_volume");
  OSPData voxelsData = ospNewData(size * size * size, OSP_UCHAR,
                                  volumetricData.data());
  ospSetData(pyramid, "voxelData", voxelsData);
  ospRelease(voxelsData);
  ospSet3i(pyramid, "dimensions", size, size, size);
  ospSetString(pyramid, "voxelType", "uchar");
  ospSet2f(pyramid, "voxelRange", 0, 255);
  ospSet3f(pyramid, "gridOrigin", -0.5f, -0.5f, -0.5f);
  ospSet3f(pyramid, "gridSpacing", 1.f / size, 1.f / size, 1.f / size);
  ospSetf(pyramid, "samplingRate", 1.f);

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ospSet2f(transferFun, "valueRange", 0, 255);
  float colors[] = {
    1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f
  };
  float opacites[] = { 0.f, 1.0f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ospSetData(transferFun, "colors", tfColorData);
  ospRelease(tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospRelease(tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(pyramid, "transferFunction", transferFun);
  ospRelease(transferFun);

  ospCommit(pyramid);

  if (renderIsosurface) {
    OSPGeometry isosurface = ospNewGeometry("isosurfaces");
    ospSetObject(isosurface, "volume", pyramid);
    ospRelease(pyramid);
    float isovalues[1] = { 0.f };
    OSPData isovaluesData = ospNewData(1, OSP_FLOAT, isovalues);
    ospSetData(isosurface, "isovalues", isovaluesData);
    ospRelease(isovaluesData);

    ospCommit(isosurface);
    AddGeometry(isosurface);
  } else {
    AddVolume(pyramid);
  }

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}


Torus::Torus()
{
  rendererType = GetParam();
}

void Torus::SetUp()
{
  Base::SetUp();

  float cam_pos[] = {-0.7f, -1.4f, 0.f};
  float cam_up[] = {0.f, 0.f, -1.f};
  float cam_view[] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);

  OSPVolume torus = CreateTorus(volumetricData, 256);

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ospSet2f(transferFun, "valueRange", -10000.f, 10000.f);
  float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  float opacites[] = { 1.0f, 1.0f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ospSetData(transferFun, "colors", tfColorData);
  ospRelease(tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospRelease(tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(torus, "transferFunction", transferFun);
  ospRelease(transferFun);

  ospCommit(torus);

  OSPGeometry isosurface = ospNewGeometry("isosurfaces");
  ospSetObject(isosurface, "volume", torus);
  ospRelease(torus);
  float isovalues[2] = { -7000.f, 0.f };
  OSPData isovaluesData = ospNewData(2, OSP_FLOAT, isovalues);
  ospSetData(isosurface, "isovalues", isovaluesData);
  ospRelease(isovaluesData);
  ospCommit(isosurface);
  AddGeometry(isosurface);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

SlicedCube::SlicedCube()
{
}

void SlicedCube::SetUp()
{
  Base::SetUp();

  float cam_pos[] = {-0.7f, -1.4f, 0.f};
  float cam_up[] = {0.f, 0.f, -1.f};
  float cam_view[] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);

  int size = 100;

  volumetricData.resize(size*size*size, 0);

  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        float X = (x / (float)size) - 0.5;
        float Y = (y / (float)size) - 0.5;
        float Z = (z / (float)size) - 0.5;

        volumetricData[size*size * x + size * y + z] = sin(30*X + 3 * sin(30*Y + 3 * sin(30*Z)));
      }
    }
  }

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ASSERT_TRUE(transferFun);
  ospSet2f(transferFun, "valueRange", -1.f, 1.f);
  float colors[] = {
    0.85f, 0.85f, 1.0f,
    0.1f, 0.0f, 0.0f
  };
  float opacites[] = { 0.0f, 1.0f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ASSERT_TRUE(tfColorData);
  ospSetData(transferFun, "colors", tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ASSERT_TRUE(tfOpacityData);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospCommit(transferFun);

  OSPVolume blob = ospNewVolume("shared_structured_volume");
  ASSERT_TRUE(blob);
  OSPData voxelsData = ospNewData(size * size * size, OSP_FLOAT,
                                  volumetricData.data(),
                                  OSP_DATA_SHARED_BUFFER);
  ASSERT_TRUE(voxelsData);
  ospSetData(blob, "voxelData", voxelsData);
  ospSet3i(blob, "dimensions", size, size, size);
  ospSetString(blob, "voxelType", "float");
  ospSet2f(blob, "voxelRange", 0.f, 3.f);
  ospSet3f(blob, "gridOrigin", -0.5f, -0.5f, -0.5f);
  ospSet3f(blob, "gridSpacing", 1.f / size, 1.f / size, 1.f / size);
  ospSetObject(blob, "transferFunction", transferFun);
  ospCommit(blob);

  OSPGeometry slice = ospNewGeometry("slices");
  ASSERT_TRUE(slice);
  float planes[] = {
    1.f, 1.f, 1.f, 0.5f,
    1.f, 1.f, 1.f, 0.f,
    1.f, 1.f, 1.f, -0.5f
  };
  OSPData planesData = ospNewData(3, OSP_FLOAT4, planes);
  ASSERT_TRUE(planesData);
  ospSetData(slice, "planes", planesData);
  ospSetObject(slice, "volume", blob);
  ospCommit(slice);
  AddGeometry(slice);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient);
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

MTLMirrors::MTLMirrors() {
  auto param = GetParam();
  Kd = std::get<0>(param);
  Ks = std::get<1>(param);
  Ns = std::get<2>(param);
  d = std::get<3>(param);
  Tf = std::get<4>(param);

  rendererType = "pathtracer";
}

void MTLMirrors::SetUp() {
  Base::SetUp();

  ospSet3f(renderer, "bgColor", 0.5f, 0.5f, 0.45f);

  OSPData data;

  float mirrorsVertices[] = {
    2.f, 1.f, 5.f,
    2.f, -1.f, 5.f,
    0.f, 1.f, 2.f,
    0.f, -1.f, 2.f,
    0.f, 1.f, 5.f,
    0.f, -1.f, 5.f,
    -2.f, 1.f, 2.f,
    -2.f, -1.f, 2.f,
  };
  int32_t mirrorsIndices[] = { 0, 1, 2, 1, 2, 3, 4, 5, 6, 5, 6, 7 };
  OSPGeometry mirrors = ospNewGeometry("triangles");
  ASSERT_TRUE(mirrors);
  data = ospNewData(8, OSP_FLOAT3, mirrorsVertices);
  ASSERT_TRUE(data);
  ospCommit(data);
  ospSetData(mirrors, "vertex", data);
  data = ospNewData(4, OSP_INT3, mirrorsIndices);
  ASSERT_TRUE(data);
  ospSetData(mirrors, "index", data);
  OSPMaterial mirrorsMaterial = ospNewMaterial2(rendererType.c_str(), "OBJMaterial");
  ASSERT_TRUE(mirrorsMaterial);
  ospSet3f(mirrorsMaterial, "Kd", Kd.x, Kd.y, Kd.z);
  ospSet3f(mirrorsMaterial, "Ks", Ks.x, Ks.y, Ks.z);
  ospSetf(mirrorsMaterial, "Ns", Ns);
  ospSetf(mirrorsMaterial, "d", d);
  ospSet3f(mirrorsMaterial, "Tf", Tf.x, Tf.y, Tf.z);
  ospCommit(mirrorsMaterial);
  ospSetMaterial(mirrors, mirrorsMaterial);
  ospCommit(mirrors);
  AddGeometry(mirrors);

  float sphereCenters[] = { 1.f, 0.f, 7.f, 0.f };
  OSPGeometry light = ospNewGeometry("spheres");
  ASSERT_TRUE(light);
  ospSetf(light, "radius", 1.f);
  data = ospNewData(1, OSP_FLOAT4, sphereCenters);
  ASSERT_TRUE(data);
  ospSetData(light, "spheres", data);
  ospSetMaterial(light, ospNewMaterial2(rendererType.c_str(), "Luminous"));
  ospCommit(light);
  AddGeometry(light);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient);
  ospSetf(ambient, "intensity", 0.01f);
  ospCommit(ambient);
  AddLight(ambient);
}

Pipes::Pipes()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  materialType = std::get<1>(params);
  radius = std::get<2>(params);

  frames = 10;
}

void Pipes::SetUp()
{
  Base::SetUp();

  float cam_pos[] = {-7.f, 2.f, 0.7f};
  float cam_view[] = {7.f, -2.f, -0.7f};
  float cam_up[] = {0.f, 0.f, 1.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up", cam_up);

  float vertex[] = {
    -2.f,  2.f, -2.f, 0.f,
     2.f,  2.f, -2.f, 0.f,
     2.f, -2.f, -2.f, 0.f,
    -2.f, -2.f, -2.f, 0.f,
    -1.f, -1.f, -1.f, 0.f,
     1.f, -1.f, -1.f, 0.f,
     1.f,  1.f, -1.f, 0.f,
    -1.f,  1.f, -1.f, 0.f,
    -1.f,  1.f,  1.f, 0.f,
     1.f,  1.f,  1.f, 0.f,
     1.f, -1.f,  1.f, 0.f,
    -1.f, -1.f,  1.f, 0.f,
    -2.f, -2.f,  2.f, 0.f,
     2.f, -2.f,  2.f, 0.f,
     2.f,  2.f,  2.f, 0.f,
    -2.f,  2.f,  2.f, 0.f,
  };

  int index[] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
  OSPGeometry streamlines = ospNewGeometry("streamlines");
  ASSERT_TRUE(streamlines);
  OSPData data = ospNewData(16, OSP_FLOAT3A, vertex);
  ASSERT_TRUE(data);
  ospSetData(streamlines, "vertex", data);
  data = ospNewData(15, OSP_INT, index);
  ASSERT_TRUE(data);
  ospSetData(streamlines, "index", data);
  ospSet1f(streamlines, "radius", radius);
  ospSetMaterial(streamlines, CreateMaterial(materialType));
  ospCommit(streamlines);

  AddGeometry(streamlines);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient);
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

TextureVolume::TextureVolume()
{
  rendererType = GetParam();
}

void TextureVolume::SetUp()
{
  Base::SetUp();

  float cam_pos[] = {-0.7f, -1.4f, 0.f};
  float cam_up[] = {0.f, 0.f, -1.f};
  float cam_view[] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);

  OSPVolume torus = CreateTorus(volumetricData, 256);

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ospSet2f(transferFun, "valueRange", -10000.f, 10000.f);
  float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  float opacites[] = { 1.0f, 1.0f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ospSetData(transferFun, "colors", tfColorData);
  ospRelease(tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospRelease(tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(torus, "transferFunction", transferFun);
  ospRelease(transferFun);
  ospCommit(torus);

  OSPMaterial sphereMaterial = ospNewMaterial2(rendererType.c_str(), "default");
  OSPTexture tex = ospNewTexture("volume");
  ospSetObject(tex, "volume", torus);
  ospCommit(tex);
  ospSetObject(sphereMaterial, "map_Kd", tex);
  ospCommit(sphereMaterial);

  float sphereVertex[] = {0.f, 0.f, 0.f, 0.0f};
  OSPGeometry sphere = ospNewGeometry("spheres");
  auto data = ospNewData(1, OSP_FLOAT4, sphereVertex);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  ospRelease(data);
  ospSet1f(sphere, "radius", 0.51f);
  ospSetMaterial(sphere, sphereMaterial);
  ospCommit(sphere);
  ospAddGeometry(world,sphere);
  ospRelease(sphereMaterial);
  ospRelease(torus);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

DepthCompositeVolume::DepthCompositeVolume()
{
  rendererType = GetParam();
}


void DepthCompositeVolume::SetUp()
{
  Base::SetUp();

  const float cam_pos[] = {0.f, 0.f, 1.0f};
  const float cam_up[] = {0.f, 1.f, 0.f};
  const float cam_view[] = {0.0f, 0.f, -1.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);

  OSPVolume torus = CreateTorus(volumetricData, 256);

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ospSet2f(transferFun, "valueRange", -10000.f, 10000.f);
  const float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float opacites[] = { 0.05f, 0.1f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ospSetData(transferFun, "colors", tfColorData);
  ospRelease(tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospRelease(tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(torus, "transferFunction", transferFun);
  ospRelease(transferFun);
  ospCommit(torus);

  AddVolume(torus);

  OSPTexture2D depthTex = ospNewTexture("texture2D");
  float* data = new float[imgSize.x*imgSize.y];
  for (size_t y = 0; y < imgSize.y; y++) {
    for (size_t x = 0; x < imgSize.x; x++) {
      const size_t index = imgSize.x*y + x;
      if (x < imgSize.x/3) {
        data[index] = 999.f;
      } else if (x < (imgSize.x*2)/3) {
        data[index] = 0.75f;
      } else {
        data[index] = 0.00001f;
      }
    }
  }
  auto ospData = ospNewData(imgSize.x*imgSize.y, OSP_FLOAT, data);
  ospCommit(ospData);
  delete[] data;
  ospSet1i(depthTex, "type", (int)OSP_TEXTURE_R32F);
  ospSet1i(depthTex, "flags", OSP_TEXTURE_FILTER_NEAREST);
  ospSet2i(depthTex, "size", imgSize.x, imgSize.y);
  ospSetObject(depthTex, "data", ospData);
  ospCommit(depthTex);
  ospRelease(ospData);

  ospSetObject(renderer, "maxDepthTexture", depthTex);
  ospRelease(depthTex);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

Subdivision::Subdivision()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  materialType = std::get<1>(params);
  frames = 10;
}

void Subdivision::SetUp()
{
  Base::SetUp();

  float inf = FLT_MAX;

  float cube_vertices[8][3] =
  {
          { -1.0f, -1.0f, -1.0f },
          {  1.0f, -1.0f, -1.0f },
          {  1.0f, -1.0f,  1.0f },
          { -1.0f, -1.0f,  1.0f },
          { -1.0f,  1.0f, -1.0f },
          {  1.0f,  1.0f, -1.0f },
          {  1.0f,  1.0f,  1.0f },
          { -1.0f,  1.0f,  1.0f }
  };

  float cube_colors[8][4] =
  {
          {  0.0f,  0.0f,  0.0f, 1.f },
          {  1.0f,  0.0f,  0.0f, 1.f },
          {  1.0f,  0.0f,  1.0f, 1.f },
          {  0.0f,  0.0f,  1.0f, 1.f },
          {  0.0f,  1.0f,  0.0f, 1.f },
          {  1.0f,  1.0f,  0.0f, 1.f },
          {  1.0f,  1.0f,  1.0f, 1.f },
          {  0.0f,  1.0f,  1.0f, 1.f }
  };

  unsigned int cube_indices[24] = {
          0, 4, 5, 1,
          1, 5, 6, 2,
          2, 6, 7, 3,
          0, 3, 7, 4,
          4, 7, 6, 5,
          0, 1, 2, 3,
  };

  unsigned int cube_faces[6] = {
          4, 4, 4, 4, 4, 4
  };

  float cube_vertex_crease_weights[8] = {
          2, 2, 2, 2, 2, 2, 2, 2
  };

  unsigned int cube_vertex_crease_indices[8] = {
          0,1,2,3,4,5,6,7
  };

  float cube_edge_crease_weights[12] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
  };

  unsigned int cube_edge_crease_indices[24] =
  {
          0,1, 1,2, 2,3, 3,0,
          4,5, 5,6, 6,7, 7,4,
          0,4, 1,5, 2,6, 3,7,
  };

  int numIndices = 24;
  int  numFaces = 6;
  int faceSize  = 4;

  auto subd = ospNewGeometry("subdivision");
  auto vertices = ospNewData(8, OSP_FLOAT3, cube_vertices);
  ospSetData(subd, "vertex", vertices);
  auto indices = ospNewData(numIndices, OSP_UINT, cube_indices);
  ospSetData(subd, "index", indices);
  auto faces = ospNewData(numFaces, OSP_UINT, cube_faces);
  ospSetData(subd, "face", faces);
  auto edge_crease_indices = ospNewData(12, OSP_UINT2, cube_edge_crease_indices);
  ospSetData(subd, "edgeCrease.index", edge_crease_indices);
  auto edge_crease_weights = ospNewData(12, OSP_FLOAT, cube_edge_crease_weights);
  ospSetData(subd, "edgeCrease.weight", edge_crease_weights);
  auto vertex_crease_indices = ospNewData(8, OSP_UINT, cube_vertex_crease_indices);
  ospSetData(subd, "vertexCrease.index", vertex_crease_indices);
  auto vertex_crease_weights = ospNewData(8, OSP_FLOAT, cube_vertex_crease_weights);
  ospSetData(subd, "vertexCrease.weight", vertex_crease_weights);
  auto colors = ospNewData(8, OSP_FLOAT4, cube_colors);
  ospSetData(subd, "color", colors);
  ospSet1f(subd, "level", 128.0f);
  ospSetMaterial(subd, CreateMaterial(materialType));

  ospCommit(subd);
  AddGeometry(subd);

  float cam_pos[] = {-1.5f, 2.f, 1.7f};
  float cam_view[] = {1.5f, -2.f, -1.7f};
  float cam_up[] = {0.f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up", cam_up);

  OSPLight directional = ospNewLight(renderer, "directional");
  ASSERT_TRUE(directional);
  ospSetf(directional, "intensity", 0.5f);
  ospSet3f(directional, "direction", -.2f, -.3f, -.4f);
  ospCommit(directional);
  AddLight(directional);
}

} // namespace OSPRayTestScenes

