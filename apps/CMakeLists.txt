## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_APPS_ENABLE_GLM
  "Build ospray_cpp glm tests/tutorial"
  OFF
  "OSPRAY_ENABLE_APPS_TESTING OR OSPRAY_ENABLE_APPS_TUTORIALS"
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
