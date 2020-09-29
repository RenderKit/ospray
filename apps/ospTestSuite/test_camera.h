// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Test all stereo modes of perspective and panoramic
class Stereo : public Base,
                  public ::testing::TestWithParam<
                      std::tuple<const char * /*camera*/, OSPStereoMode>>
{
 public:
  Stereo();
  void SetUp() override;

 protected:
  std::string cameraType;
  vec3f pos{0.0f, 0.0f, -0.5f};
  OSPStereoMode stereoMode;
};

} // namespace OSPRayTestScenes
