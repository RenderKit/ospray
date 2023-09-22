// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef SYCL_LANGUAGE_VERSION

#include <ospray/ospray_util.h>
#include <cmath>
#include <random>
#include <vector>
#include "test_fixture.h"

extern OSPRayEnvironment *ospEnv;
#include "sycl/sycl.hpp"

namespace OSPRayTestScenes {

// Fixture class for a unit test of gpu capable, data sharing
class TestUSMSharing
    : public Base,
      public ::testing::TestWithParam<const char * /*memory type*/>
{
 public:
  TestUSMSharing();
  ~TestUSMSharing();
  void SetUp() override;

 protected:
  std::string memoryType; // host, shared, device
 private:
  std::vector<float> voxels;
  float *deviceVoxels = nullptr;
};

TestUSMSharing::TestUSMSharing()
{
  memoryType = GetParam();
}

TestUSMSharing::~TestUSMSharing()
{
  sycl::queue *appsSyclQueue = ospEnv->GetAppsSyclQueue();
  if (appsSyclQueue)
    free(deviceVoxels, *appsSyclQueue);
}

void TestUSMSharing::SetUp()
{
  Base::SetUp();

  vec3i dims{100, 100, 100};
  size_t numVoxels = dims.long_product();
  voxels = std::vector<float>(numVoxels);
  for (int x = 0; x < dims.x; ++x) {
    double dx = (double)x / (double)dims.x - 0.5;
    for (int y = 0; y < dims.y; ++y) {
      double dy = (double)y / (double)dims.y - 0.5;
      for (int z = 0; z < dims.z; ++z) {
        double dz = (double)z / (double)dims.z - 0.5;
        voxels[x * dims.y * dims.z + y * dims.z + z] =
            1.0 - sqrt(dx * dx + dy * dy + dz * dz);
      }
    }
  }

  deviceVoxels = voxels.data();
  sycl::queue *appsSyclQueue = ospEnv->GetAppsSyclQueue();
  if (appsSyclQueue) {
    if (memoryType == "host") {
      deviceVoxels = sycl::malloc_host<float>(numVoxels, *appsSyclQueue);
    } else if (memoryType == "device") {
      deviceVoxels = sycl::malloc_device<float>(numVoxels, *appsSyclQueue);
    } else {
      deviceVoxels = sycl::malloc_shared<float>(numVoxels, *appsSyclQueue);
    }
    appsSyclQueue->memcpy(
        deviceVoxels, voxels.data(), numVoxels * sizeof(float));
    appsSyclQueue->wait();
  }

  // create and setup camera
  camera.setParam("position", vec3f(200.f, 50.f, 50.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));
  camera.setParam("direction", vec3f(-1.f, 0.f, 0.f));

  // create and setup model and mesh
  ospray::cpp::Volume volume("structuredRegular");
  volume.setParam("data", cpp::SharedData(deviceVoxels, dims));
  volume.commit();

  ospray::cpp::TransferFunction tf("piecewiseLinear");
  std::vector<vec3f> color = {vec3f(0.5f, 0.0f, 0.0f),
      vec3f(0.0f, 0.5f, 0.0f),
      vec3f(0.0f, 0.0f, 1.0f)};
  std::vector<float> opacity = {0.05f, 0.0f, 0.1f, 0.0f, 0.2f, 0.0f, 1.0f};

  tf.setParam("value", box1f(0.14f, 1.0f));
  tf.setParam("color", ospray::cpp::CopiedData(color));
  tf.setParam("opacity", ospray::cpp::CopiedData(opacity));
  tf.commit();

  ospray::cpp::VolumetricModel vm(volume);
  vm.setParam("transferFunction", tf);
  vm.commit();

  AddModel(vm);

  cpp::Light distant("distant");
  distant.setParam("intensity", 3.0f);
  distant.setParam("direction", vec3f(0.3f, -4.0f, 0.8f));
  distant.setParam("color", vec3f(1.0f, 0.5f, 0.5f));
  distant.setParam("angularDiameter", 1.0f);
  AddLight(distant);

  cpp::Light ambient = ospNewLight("ambient");
  ambient.setParam("intensity", 0.1f);
  AddLight(ambient);
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(TestUSMSharing, structured_regular)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    SharedData, TestUSMSharing, ::testing::Values("host", "shared", "device"));
} // namespace OSPRayTestScenes
#endif
