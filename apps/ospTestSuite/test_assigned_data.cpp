// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <ospray/ospray_util.h>
#include <random>
#include "test_fixture.h"
#ifdef SYCL_LANGUAGE_VERSION
#include "sycl/sycl.hpp"
#endif

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

struct TestAssignedCopyDelCounts
{
  ~TestAssignedCopyDelCounts();
  static int delcounts;
};

int TestAssignedCopyDelCounts::delcounts{0};

static void exiting()
{
  ASSERT_EQ(TestAssignedCopyDelCounts::delcounts, 2)
      << "Mem leak: The buffers should have been freed by now.";
}

TestAssignedCopyDelCounts::~TestAssignedCopyDelCounts()
{
  exiting();
}

class TestAssignedCopy : public Base,
                         public ::testing::Test,
                         virtual public TestAssignedCopyDelCounts
{
 public:
  void SetUp() override;
};

#define VALIDATION_MESSAGE                                                     \
  std::cout << __FUNCTION__                                                    \
            << " delcounts = " << TestAssignedCopyDelCounts::delcounts         \
            << std::endl

static void customMallocDeleter(const void *dels, const void *buff)
{
  free((void *)buff);
  (*(int *)dels)++;
  VALIDATION_MESSAGE;
}

#ifdef SYCL_LANGUAGE_VERSION
static void customSYCLDeleter(const void *syclQueue, const void *buff)
{
  sycl::free((void *)buff, *(sycl::queue *)syclQueue);
  TestAssignedCopyDelCounts::delcounts++;
  VALIDATION_MESSAGE;
}
#endif

static void customNewDeleter(const void *dels, const void *buff)
{
  delete[] (float *)buff;
  (*(int *)dels)++;
  VALIDATION_MESSAGE;
}
void TestAssignedCopy::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(0.f, 0.f, 4.f));
  camera.setParam("direction", vec3f(0.f, 0.f, -1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  cpp::Geometry spheres("sphere");

  const int numspheres = 1000;
  std::vector<vec3f> centersB;

  // per default use malloc
  float *radiiB = (float *)malloc(numspheres * sizeof(float));
  OSPDeleterCallback radiiDeleter = customMallocDeleter;
  void *radiiUserData = &TestAssignedCopyDelCounts::delcounts;
#ifdef SYCL_LANGUAGE_VERSION
  auto *appsSyclQueue = ospEnv->GetAppsSyclQueue();
  if (appsSyclQueue) { // but on GPU use sycl::malloc
    free(radiiB);
    radiiB = sycl::malloc_shared<float>(numspheres, *appsSyclQueue);
    radiiDeleter = customSYCLDeleter;
    radiiUserData = appsSyclQueue;
  }
#endif

  vec2f *tcoordsB = new vec2f[numspheres];

  unsigned int randomSeed{0};
  std::mt19937 gen(randomSeed);
  std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
  std::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);

  for (int i = 0; i < numspheres; i++) {
    radiiB[i] = radiusDistribution(gen);
    const float x = centerDistribution(gen);
    const float y = centerDistribution(gen);
    const float z = centerDistribution(gen);
    centersB.emplace_back(x, y, z);
    tcoordsB[i] = vec2f((float)i / (float)numspheres, 0.f);
  }

  OSPData radii = ospNewAssignedData1D(
      radiiB, OSP_FLOAT, numspheres, radiiDeleter, radiiUserData);

  OSPData tcoords = ospNewAssignedData1D(tcoordsB,
      OSP_VEC2F,
      numspheres,
      customNewDeleter,
      &TestAssignedCopyDelCounts::delcounts);

  spheres.setParam("sphere.position", cpp::MovedData(centersB));

  EXPECT_TRUE(centersB.empty()) << "Buffer was not moved.";

  spheres.setParam("sphere.radius", radii);
  spheres.setParam("sphere.texcoord", tcoords);
  spheres.commit();

  ASSERT_EQ(delcounts, 0) << "Buffers were freed before the app released them.";

  ospRelease(radii);
  ospRelease(tcoords);

  cpp::GeometricModel model(spheres);

  cpp::Material sphereMaterial("obj");
  sphereMaterial.commit();

  model.setParam("material", sphereMaterial);

  AddModel(model);

  cpp::Light distant("distant");
  distant.setParam("intensity", 3.0f);
  distant.setParam("direction", vec3f(0.3f, -4.0f, 0.8f));
  distant.setParam("color", vec3f(1.0f, 0.5f, 0.5f));
  distant.setParam("angularDiameter", 1.0f);
  AddLight(distant);

  cpp::Light ambient = ospNewLight("ambient");
  ambient.setParam("intensity", 0.1f);
  AddLight(ambient);

  if (delcounts > 0)
    std::cout << "Buffers were already freed "
                 "(internal copy? missing internal Ref<>?)."
              << std::endl;
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_F(TestAssignedCopy, deleterCallbacks)
{
  PerformRenderTest();
}

} // namespace OSPRayTestScenes
