## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_MODULE_DENOISER
  "Build the Denoiser FrameOp module for denoising (requires OIDN)"
  OFF
  OSPRAY_MODULE_CPU
  OFF
)

if (NOT OSPRAY_MODULE_DENOISER)
  return()
endif()

include(GenerateExportHeader)

add_library(ospray_module_denoiser SHARED ${OSPRAY_RESOURCE} DenoiseFrameOp.cpp)

generate_export_header(ospray_module_denoiser)

target_link_libraries(ospray_module_denoiser
PRIVATE
  OpenImageDenoise
  ospray_module_cpu
)

target_include_directories(ospray_module_denoiser
PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
)

ospray_install_library(ospray_module_denoiser lib)
