// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

using OSPRayTestScenes::FromOsprayTesting;
using OSPRayTestScenes::RendererMaterialList;
using OSPRayTestScenes::Texture2D;

using namespace ospcommon;

TEST_P(RendererMaterialList, material_list)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(MaterialLists,
    RendererMaterialList,
    ::testing::Values("scivis", "pathtracer"));

INSTANTIATE_TEST_CASE_P(TestScenesPtMaterials,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("test_pt_glass",
                           "test_pt_luminous",
                           "test_pt_metal_roughness",
                           "test_pt_metallic_flakes",
                           "test_pt_obj"),
        ::testing::Values("pathtracer")));

TEST_P(Texture2D, filter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Appearance,
    Texture2D,
    ::testing::Values(std::make_tuple(OSP_TEXTURE_FILTER_BILINEAR, true),
        std::make_tuple(OSP_TEXTURE_FILTER_NEAREST, true),
        std::make_tuple(OSP_TEXTURE_FILTER_NEAREST, false)));
