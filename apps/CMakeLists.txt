## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

option(OSPRAY_APPS_ENABLE_GLM "Build ospray_cpp glm tests/tutorial" OFF)

if (OSPRAY_APPS_ENABLE_GLM)
  find_package(glm REQUIRED)
  if(TARGET glm::glm)
    set(GLM_TARGET glm::glm)
  else()
    set(GLM_TARGET glm)
  endif()
endif()

add_all_subdirectories()
