## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_TUTORIALS)
  return()
endif()

# build ospTutorial, for demonstration and API testing
add_executable(ospTutorial ${OSPRAY_RESOURCE} ospTutorial.c)
target_link_libraries(ospTutorial PRIVATE ospray)
ospray_sign_target(ospTutorial)

# C++ version
add_executable(ospTutorialCpp ${OSPRAY_RESOURCE} ospTutorial.cpp)
target_link_libraries(ospTutorialCpp PRIVATE ospray_sdk)
ospray_sign_target(ospTutorialCpp)

# async version
add_executable(ospTutorialAsync ${OSPRAY_RESOURCE} ospTutorialAsync.c)
target_link_libraries(ospTutorialAsync PRIVATE ospray)
ospray_sign_target(ospTutorialAsync)

install(TARGETS ospTutorial ospTutorialCpp ospTutorialAsync
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)

# GLM C++ version

if (OSPRAY_APPS_ENABLE_GLM)
  add_executable(ospTutorialGLM ${OSPRAY_RESOURCE} ospTutorialGLM.cpp)
  target_link_libraries(ospTutorialGLM PRIVATE ospray ${GLM_TARGET})
  install(TARGETS ospTutorialGLM
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT apps
  )
endif()
