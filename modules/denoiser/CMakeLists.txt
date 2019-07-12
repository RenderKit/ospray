option(OSPRAY_MODULE_DENOISER
  "Build the Denoiser FrameOp module for denoising (requires OIDN)" OFF)

if (OSPRAY_MODULE_DENOISER)
  include(GenerateExportHeader)
  find_package(OpenImageDenoise 0.9 REQUIRED)

  ospray_create_library(ospray_module_denoiser
    DenoiseFrameOp.cpp
  )

  generate_export_header(ospray_module_denoiser)

  target_link_libraries(ospray_module_denoiser
  PRIVATE
    OpenImageDenoise
  PUBLIC
    ospray_common
    ospray_module_ispc
  )

endif()

