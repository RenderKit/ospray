#include "ospray_test_fixture.h"

using OSPRayTestScenes::SlicedCube;

TEST_F(SlicedCube, simple) {
  PerformRenderTest();
}

