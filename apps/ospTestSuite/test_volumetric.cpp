// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

using OSPRayTestScenes::DepthCompositeVolume;
using OSPRayTestScenes::FromOsprayTesting;
using OSPRayTestScenes::TextureVolume;

INSTANTIATE_TEST_CASE_P(TestScenesVolumes,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("gravity_spheres_volume",
                           "perlin_noise_volumes",
                           "unstructured_volume"),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(TextureVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, TextureVolume, ::testing::Values("scivis"));

TEST_P(DepthCompositeVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers,
    DepthCompositeVolume,
    ::testing::Combine(::testing::Values("scivis"),
        ::testing::Values(vec4f(0.f),
            vec4f(1.f),
            vec4f(0.f, 0.f, 0.f, 1.f),
            vec4f(1.f, 0.f, 0.f, 0.5f))));
