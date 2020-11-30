## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_APPS_BENCHMARK
  "Enable building, installing, and packaging of benchmark tools."
  ON
  OSPRAY_ENABLE_APPS
  OFF
)

cmake_dependent_option(
  OSPRAY_APPS_EXAMPLES
  "Enable building, installing, and packaging of ospExamples."
  ON
  OSPRAY_ENABLE_APPS
  OFF
)

cmake_dependent_option(
  OSPRAY_APPS_TUTORIALS
  "Enable building, installing, and packaging of tutorial apps."
  ON
  OSPRAY_ENABLE_APPS
  OFF
)

cmake_dependent_option(
  OSPRAY_APPS_TESTING
  "Enable building, installing, and packaging of test tools."
  ON
  OSPRAY_ENABLE_APPS
  OFF
)

cmake_dependent_option(
  OSPRAY_APPS_ENABLE_GLM
  "Build ospray_cpp glm tests/tutorial"
  OFF
  "OSPRAY_APPS_TESTING OR OSPRAY_APPS_TUTORIALS"
  OFF
)

# Find GLM here for both ospTutorialGLM and ospTestSuite
if (OSPRAY_APPS_ENABLE_GLM)
  find_package(glm REQUIRED)
  if(TARGET glm::glm)
    set(GLM_TARGET glm::glm)
  else()
    set(GLM_TARGET glm)
  endif()
endif()

add_all_subdirectories()
