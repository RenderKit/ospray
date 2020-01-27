## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

option(OSPRAY_MODULE_DENOISER
  "Build the Denoiser FrameOp module for denoising (requires OIDN)" OFF)

if (NOT OSPRAY_MODULE_DENOISER)
  return()
endif()

include(GenerateExportHeader)

add_library(ospray_module_denoiser SHARED DenoiseFrameOp.cpp)

generate_export_header(ospray_module_denoiser)

target_link_libraries(ospray_module_denoiser
PRIVATE
  OpenImageDenoise
  ospray_module_ispc
)

target_include_directories(ospray_module_denoiser
PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
)

ospray_install_library(ospray_module_denoiser lib)
