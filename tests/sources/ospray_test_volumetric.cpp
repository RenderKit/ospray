#include "ospray_test_fixture.h"

using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::Torus;

TEST_P(Sierpinski, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Volumetric, Sierpinski, ::testing::Combine(::testing::Values("scivis"), ::testing::Values(false), ::testing::Values(4, 6, 9)));
INSTANTIATE_TEST_CASE_P(Isosurfaces, Sierpinski, ::testing::Combine(::testing::Values("scivis", "pathtracer"), ::testing::Values(true), ::testing::Values( 5, 7,  9)));

TEST_P(Torus, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, Torus, ::testing::Values("scivis", "pathtracer"));

