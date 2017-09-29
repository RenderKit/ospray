#include "ospray_test_fixture.h"

#include <cmath>

extern OSPRayEnvironment * ospEnv;

namespace OSPRayTestScenes {

Base::Base() {
  const ::testing::TestCase* const testCase = ::testing::UnitTest::GetInstance()->current_test_case();
  const ::testing::TestInfo* const testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
  imgSize = ospEnv->GetImgSize();

  testName = std::string(testCase->name()) + "__" + testInfo->name();
  for (char& byte : testName)
    if (byte == '/')
      byte = '_';

  rendererType = "scivis";
  frames = 1;
}

Base::~Base() {
}

void Base::SetUp() {
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());
}

void Base::TearDown() {
}

void Base::AddLight(OSPLight light) {
  lightsList.push_back(light);
  SetLights();
}

void Base::AddGeometry(OSPGeometry new_geometry) {
  ospAddGeometry(world, new_geometry);
  ospCommit(world);
  ospCommit(renderer);
}

void Base::AddVolume(OSPVolume new_volume) {
  ospAddVolume(world, new_volume);
  ospCommit(world);
  ospCommit(renderer);
}

void Base::PerformRenderTest() {
  RenderFrame(OSP_FB_COLOR | OSP_FB_ACCUM);
  uint32_t* framebuffer_data = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);

  if(ospEnv->GetDumpImg()) {
    std::string fileName = ospEnv->GetBaselineDir() + GetTestName() + ".ppm";
    EXPECT_EQ(writeImg(fileName, imgSize, framebuffer_data), OsprayStatus::Ok);
  } else {
    EXPECT_EQ(compareImgWithBaseline(GetImgSize(), framebuffer_data, GetTestName()), OsprayStatus::Ok);
  }

  ospUnmapFrameBuffer(framebuffer_data, framebuffer);
}

void Base::CreateEmptyScene() {
  SetCamera();
  SetWorld();
  SetRenderer();
  SetLights();
  SetFramebuffer();
}

void Base::SetCamera() {
  float cam_pos[] = {0.f, 0.f, 0.f};
  float cam_up [] = {0.f, 1.f, 0.f};
  float cam_view [] = {0.f, 0.f, 1.f};

  camera = ospNewCamera("perspective");

  ospSetf(camera, "aspect", imgSize.x/(float)imgSize.y);
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);
  ospCommit(camera);
}

void Base::SetWorld() {
  world = ospNewModel();
  ospCommit(world);
}

void Base::SetLights() {
  lights = ospNewData(lightsList.size(), OSP_LIGHT, lightsList.data());
  ospSetObject(renderer, "lights", lights);
  ospCommit(lights);
  ospCommit(renderer);
}

void Base::SetRenderer() {
  renderer = ospNewRenderer(rendererType.data());

  ospSet1i(renderer, "aoSamples", 1);
  ospSet1f(renderer, "bgColor", 1.0f);
  ospSetObject(renderer, "model",  world);
  ospSetObject(renderer, "camera", camera);
  ospCommit(renderer);
}

void Base::SetFramebuffer() {
  framebuffer = ospNewFrameBuffer(imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
  ospFrameBufferClear(framebuffer, OSP_FB_COLOR);
}

void Base::RenderFrame(const uint32_t frameBufferChannels) {
  for (int frame = 0; frame < frames; ++frame)
    ospRenderFrame(framebuffer, renderer, frameBufferChannels);
}


SingleObject::SingleObject() {
  auto params = GetParam();
  rendererType = std::get<0>(params);
  materialType = std::get<1>(params);
}

void SingleObject::SetUp() {
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());

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

void SingleObject::SetMaterial() {
  material = ospNewMaterial(renderer, materialType.data());

  if (materialType == "OBJMaterial") {
  } else if (materialType == "Glass") {
    ospSetf(material, "eta", 1.5);
    ospSet3f(material, "attenuationColor", 0.f, 1.f, 1.f);
    ospSetf(material, "attenuationDistance", 5.0);
  } else if (materialType == "Luminous") {
    ospSetf(material, "intensity", 3.0);
  } else {
    FAIL();
  }

  ospCommit(material);
}

Box::Box() {
  rendererType = "pathtracer";

  auto params = GetParam();
  cuboidMaterialType = std::get<0>(params);
  sphereMaterialType = std::get<1>(params);
}

void Box::SetUp() {
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());

  SetMaterials();

  OSPData data;

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
  data = ospNewData(16, OSP_FLOAT3, wallsVertices);
  ospCommit(data);
  ospSetData(wallsMesh, "vertex", data);
  data = ospNewData(16, OSP_FLOAT4, wallsColors);
  ospCommit(data);
  ospSetData(wallsMesh, "vertex.color", data);
  data = ospNewData(10, OSP_INT3, wallsIndices);
  ospCommit(data);
  ospSetData(wallsMesh, "index", data);
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
  data = ospNewData(2, OSP_INT3, lightIndices);
  ospSetData(lightSquare, "index", data);
  OSPMaterial lightMaterial = ospNewMaterial(renderer, "Luminous");
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
  data = ospNewData(12, OSP_INT3, cuboidIndices);
  ospCommit(data);
  ospSetData(cuboid, "index", data);
  ospSetMaterial(cuboid, cuboidMaterial);
  ospCommit(cuboid);
  AddGeometry(cuboid);

  float sphereVertex[] = { -0.3f, -0.55f, 2.5f, 0.0f};
  OSPGeometry sphere = ospNewGeometry("spheres");
  data = ospNewData(1, OSP_FLOAT4, sphereVertex);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  ospSet1f(sphere, "radius", 0.45f);
  ospSetMaterial(sphere, sphereMaterial);
  ospCommit(sphere);
  AddGeometry(sphere);
}

void Box::SetMaterials() {
  cuboidMaterial = GetMaterial(cuboidMaterialType);
  sphereMaterial = GetMaterial(sphereMaterialType);
}

OSPMaterial Box::GetMaterial(std::string type)  {
  OSPMaterial newMaterial = ospNewMaterial(renderer, type.data());
  if (type == "OBJMaterial") {
    ospSetf(newMaterial, "Ns", 100.f);
  } else if (type == "Glass") {
    ospSetf(newMaterial, "eta", 1.4);
  } else if (type == "Luminous") {
    ospSetf(newMaterial, "intensity", 0.7f);
  }
  ospCommit(newMaterial);

  return newMaterial;
}


Sierpinski::Sierpinski() {
  auto params = GetParam();
  renderIsosurface = std::get<1>(params);
  level = std::get<2>(params);

  rendererType = std::get<0>(params);
  frames = 5;
}

void Sierpinski::SetUp() {
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());

  float cam_pos[] = {-0.5f, -1.f, 0.2f};
  float cam_up [] = {0.f, 0.f, -1.f};
  float cam_view [] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);
  ospCommit(camera);
  ospCommit(renderer);

  int size = 1 << level;

  unsigned char* volumetricData = (unsigned char*)malloc(size * size * size * sizeof(unsigned char));
  std::memset(volumetricData, 0, size * size * size);

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
  OSPData voxelsData = ospNewData(size * size * size, OSP_UCHAR, (unsigned char*)volumetricData, OSP_DATA_SHARED_BUFFER);
  ospSetData(pyramid, "voxelData", voxelsData);
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
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(pyramid, "transferFunction", transferFun);

  ospCommit(pyramid);

  if (renderIsosurface) {
    OSPGeometry isosurface = ospNewGeometry("isosurfaces");
    ospSetObject(isosurface, "volume", pyramid);
    float isovalues[1] = { 0.f };
    OSPData isovaluesData = ospNewData(1, OSP_FLOAT, isovalues);
    ospSetData(isosurface, "isovalues", isovaluesData);

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


Torus::Torus() {
  rendererType = GetParam();
}

void Torus::SetUp() {
  ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());

  float cam_pos[] = {-0.7f, -1.4f, 0.f};
  float cam_up[] = {0.f, 0.f, -1.f};
  float cam_view[] = {0.5f, 1.f, 0.f};
  ospSet3fv(camera, "pos", cam_pos);
  ospSet3fv(camera, "dir", cam_view);
  ospSet3fv(camera, "up",  cam_up);
  ospCommit(camera);
  ospCommit(renderer);

  ospSet1f(renderer, "epsilon", 0.01);
  ospCommit(renderer);

  int size = 250;

  float* volumetricData = (float*)malloc(size * size * size * sizeof(float));
  std::memset(volumetricData, 0, size * size * size);

  float r = 30;
  float R = 80;

  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        float X = x - size / 2;
        float Y = y - size / 2;
        float Z = z - size / 2;

        float d = (R - std::sqrt(X*X + Y*Y));
        volumetricData[size*size * x + size * y + z] = r*r - d*d - Z*Z;
      }
    }
  }

  OSPVolume torus = ospNewVolume("shared_structured_volume");
  OSPData voxelsData = ospNewData(size * size * size, OSP_FLOAT, volumetricData, OSP_DATA_SHARED_BUFFER);
  ospSetData(torus, "voxelData", voxelsData);
  ospSet3i(torus, "dimensions", size, size, size);
  ospSetString(torus, "voxelType", "float");
  ospSet2f(torus, "voxelRange", -10000.f, 10000.f);
  ospSet3f(torus, "gridOrigin", -0.5f, -0.5f, -0.5f);
  ospSet3f(torus, "gridSpacing", 1.f / size, 1.f / size, 1.f / size);

  OSPTransferFunction transferFun = ospNewTransferFunction("piecewise_linear");
  ospSet2f(transferFun, "valueRange", -10000.f, 10000.f);
  float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  float opacites[] = { 1.0f, 1.0f };
  OSPData tfColorData = ospNewData(2, OSP_FLOAT3, colors);
  ospSetData(transferFun, "colors", tfColorData);
  OSPData tfOpacityData = ospNewData(2, OSP_FLOAT, opacites);
  ospSetData(transferFun, "opacities", tfOpacityData);
  ospCommit(transferFun);
  ospSetObject(torus, "transferFunction", transferFun);

  ospCommit(torus);

  OSPGeometry isosurface = ospNewGeometry("isosurfaces");
  ospSetObject(isosurface, "volume", torus);
  float isovalues[2] = { -7000.f, 0.f };
  OSPData isovaluesData = ospNewData(2, OSP_FLOAT, isovalues);
  ospSetData(isosurface, "isovalues", isovaluesData);
  ospCommit(isosurface);
  AddGeometry(isosurface);

  OSPLight ambient = ospNewLight(renderer, "ambient");
  ASSERT_TRUE(ambient) << "Failed to create lights";
  ospSetf(ambient, "intensity", 0.5f);
  ospCommit(ambient);
  AddLight(ambient);
}

} // namespace OSPRayTestScenes

