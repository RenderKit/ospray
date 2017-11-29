#include "ospray_test_fixture.h"

using OSPRayTestScenes::MTLMirrors;

TEST_P(MTLMirrors, Properties) {
  PerformRenderTest();
}

// the quads are mirror-like and you can see the reflection of the sphere
INSTANTIATE_TEST_CASE_P(Shiny, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 0.f, 0.f}),
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 1.f, 0.f}),
  ::testing::Values(1000.f, 1000000.f),
  ::testing::Values(1.f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}))
));

// the quads are slightly transparent
INSTANTIATE_TEST_CASE_P(Transparent, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f})),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f})),
  ::testing::Values(1.f),
  ::testing::Values(0.9, 0.99f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}), osp::vec3f({1.f, 0.f, 0.f})
));

// the quads use diffuse material
INSTANTIATE_TEST_CASE_P(Diffuse, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 1.f, 0.f})),
  ::testing::Values(osp::vec3f({0.4f, 0.4f, 0.4f})),
  ::testing::Values(1.f),
  ::testing::Values(1.f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}))
));

