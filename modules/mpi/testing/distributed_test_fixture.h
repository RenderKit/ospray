// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace MPIDistribOSPRayTestScenes {

// Base class for all distributed test fixtures
// The base class assumes the data is partitioned in a 3D
// grid with each rank assigned a cell in this grid that it owns.
// The base class will set the world region parameter to match this
class MPIDistribBase : public OSPRayTestScenes::Base
{
 public:
  MPIDistribBase();

  // We override these to get the bounds of objects as they're added
  // so we can set the regions parameter on the world.
  virtual void AddInstance(cpp::Instance instance) override;

  virtual void PerformRenderTest();

 protected:
  int mpiRank;
  int mpiSize;
  vec3i rankGridDims;
  vec3i rankBrickId;
  box3f worldBounds;
};

// Fixture class used for tests that uses 'ospray_testing' scenes
class MPIFromOsprayTesting
    : public MPIDistribBase,
      public ::testing::TestWithParam<
          std::tuple<const char * /*scene name*/, unsigned int /*spp*/>>
{
 public:
  MPIFromOsprayTesting();
  void SetUp() override;

 protected:
  std::string sceneName;
};

} // namespace MPIDistribOSPRayTestScenes
