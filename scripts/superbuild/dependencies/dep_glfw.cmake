## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME glfw)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/build
  URL "https://github.com/glfw/glfw/archive/refs/tags/3.5.1.zip"
  URL_HASH "SHA256=e9a80355e8a0c59b15ae8576c2c3aeae792c2b1082ec426dc93bde70d5017fda"
  # `patch` is not available on all systems, so use `git apply` instead. Note
  # that we initialize a Git repo in the GLFW download directory to allow the
  # Git patching approach to work. Also note that we don't want to actually
  # check out the GLFW Git repo, since we want our GLFW_HASH security checks
  # to still function correctly.
  PATCH_COMMAND git init -q . && git apply -v --ignore-space-change -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw.patch
  CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} # GLFW is a C project
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
    -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
    -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
    -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
    -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DCMAKE_C_FLAGS=-D_GLFW_BUILD_DLL # merge into ospray_imgui.dll
    -DGLFW_BUILD_DOCS=OFF
    -DGLFW_BUILD_EXAMPLES=OFF
    -DGLFW_BUILD_TESTS=OFF
    -DGLFW_BUILD_WAYLAND=OFF # avoid wayland-scanner and libwayland dependency
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS ${ALWAYS_REBUILD}
)

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
