## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# NOTE: This CMakeLists.txt is intended to be used to exercise an OSPRay
#       install and demonstrate how external applications can build against
#       OSPRay using CMake.
#
#       Once you have done a 'make install' of an existing OSPRay
#       build (will install to CMAKE_INSTALL_PREFIX), create a separate build
#       directory and invoke CMake on this directory. If you have 'ospray_DIR'
#       setup correctly to point to where you just installed OSPRay, then this
#       should build the ospTutorial app from that install and NOT use your
#       local build.

cmake_minimum_required(VERSION 3.1)

project(ospTutorialFindospray LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(ospray 2.0.0 REQUIRED)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../ospTutorial.c)
target_link_libraries(${PROJECT_NAME} ospray::ospray)
