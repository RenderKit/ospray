## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(ospray_add_dependent_lib_configuration TARGET_NAME VAR CONFIGURATION)
    get_target_property(LIBRARY ${TARGET_NAME} IMPORTED_LOCATION_${CONFIGURATION})
    get_target_property(LIBRARY_SO ${TARGET_NAME} IMPORTED_SONAME_${CONFIGURATION})
    if (LIBRARY_SO)
      get_filename_component(LIBRARY_DIR ${LIBRARY} DIRECTORY)
      get_filename_component(LIBRARY_NAME ${LIBRARY_SO} NAME)
      set(LIBRARY "${LIBRARY_DIR}/${LIBRARY_NAME}")
    endif()
    list(APPEND ${VAR} ${LIBRARY})
endmacro()

macro(ospray_add_dependent_lib TARGET_NAME)
  if (TARGET ${TARGET_NAME})
    get_target_property(CONFIGURATIONS ${TARGET_NAME} IMPORTED_CONFIGURATIONS)
    # first/default configuration
    list(GET CONFIGURATIONS 0 CONFIGURATION)
    ospray_add_dependent_lib_configuration(${TARGET_NAME} DEPENDENT_LIBS ${CONFIGURATION})
    # potentially Debug configuration
    list(FIND CONFIGURATIONS "DEBUG" FOUND)
    if(FOUND GREATER 0)
      ospray_add_dependent_lib_configuration(${TARGET_NAME} DEPENDENT_LIBS_DEBUG "DEBUG")
    else()
      ospray_add_dependent_lib_configuration(${TARGET_NAME} DEPENDENT_LIBS_DEBUG ${CONFIGURATION})
    endif()
  else()
    message(STATUS "Skipping target '${TARGET_NAME}'")
  endif()
endmacro()

macro(ospray_add_dependent_lib_plugins TARGET_NAME PLUGINS_PATTERN VER_PATTERN)
  if (TARGET ${TARGET_NAME})
    # retrieve library directory
    get_target_property(CONFIGURATIONS ${TARGET_NAME} IMPORTED_CONFIGURATIONS)
    list(GET CONFIGURATIONS 0 CONFIGURATION) # use first/default configuration
    get_target_property(LIBRARY ${TARGET_NAME} IMPORTED_LOCATION_${CONFIGURATION})
    get_filename_component(LIBRARY_DIR ${LIBRARY} DIRECTORY)

    # search for plugins with given file pattern
    if (WIN32)
      file(GLOB LIBRARY_PLUGINS LIST_DIRECTORIES FALSE
        "${LIBRARY_DIR}/${PLUGINS_PATTERN}.dll"
      )
    elseif (APPLE)
      file(GLOB LIBRARY_PLUGINS LIST_DIRECTORIES FALSE
        "${LIBRARY_DIR}/lib${PLUGINS_PATTERN}${VER_PATTERN}.dylib"
      )
    else()
      file(GLOB LIBRARY_PLUGINS LIST_DIRECTORIES FALSE
        "${LIBRARY_DIR}/lib${PLUGINS_PATTERN}.so${VER_PATTERN}"
      )
    endif()

    # iterate over all found plugins and add them to DEPENDENT_LIBS list
    foreach(LIBRARY_PLUGIN ${LIBRARY_PLUGINS})
      get_filename_component(LIBRARY_NAME ${LIBRARY_PLUGIN} NAME_WE)
      if (LIBRARY_NAME MATCHES "_debug")
        list(APPEND DEPENDENT_LIBS_DEBUG ${LIBRARY_PLUGIN})
      else()
        list(APPEND DEPENDENT_LIBS ${LIBRARY_PLUGIN})
      endif()
    endforeach()
  else()
    message(STATUS "Skipping target '${TARGET_NAME}' plugins")
  endif()
endmacro()

ospray_add_dependent_lib(ispcrt::ispcrt)
ospray_add_dependent_lib_plugins(ispcrt::ispcrt "ispcrt_device_*" "")
ospray_add_dependent_lib(rkcommon::rkcommon)
if (RKCOMMON_TASKING_TBB)
  ospray_add_dependent_lib(TBB::tbb)
  ospray_add_dependent_lib(TBB::tbbmalloc)
  ospray_add_dependent_lib_plugins(TBB::tbb "tbbbind" ".[0-9]")
  ospray_add_dependent_lib_plugins(TBB::tbb "tbbbind_?_?" ".[0-9]")
endif()
ospray_add_dependent_lib(embree)
ospray_add_dependent_lib(openvkl::openvkl)
ospray_add_dependent_lib(openvkl::openvkl_module_cpu_device)
ospray_add_dependent_lib(openvkl::openvkl_module_cpu_device_4)
ospray_add_dependent_lib(openvkl::openvkl_module_cpu_device_8)
ospray_add_dependent_lib(openvkl::openvkl_module_cpu_device_16)
if (OSPRAY_MODULE_DENOISER)
  ospray_add_dependent_lib(OpenImageDenoise)
  ospray_add_dependent_lib(OpenImageDenoise_core)
  ospray_add_dependent_lib_plugins(OpenImageDenoise "OpenImageDenoise_device_*" ".[0-9].[0-9].[0-9]")
endif()
if (OSPRAY_MODULE_GPU OR OSPRAY_MODULE_DENOISER)
  if (OSPRAY_MODULE_GPU)
    get_filename_component(SYCL_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    if (NOT WIN32)
      set(SYCL_DIR "${SYCL_DIR}/../lib")
    endif()
  else()
    get_target_property(CONFIGURATIONS OpenImageDenoise IMPORTED_CONFIGURATIONS)
    list(GET CONFIGURATIONS 0 CONFIGURATION) # use first/default configuration
    get_target_property(OIDN_LIB OpenImageDenoise IMPORTED_LOCATION_${CONFIGURATION})
    get_filename_component(SYCL_DIR ${OIDN_LIB} DIRECTORY)
  endif()

  if (WIN32)
    file(GLOB SYCL_LIB LIST_DIRECTORIES FALSE
      "${SYCL_DIR}/sycl?.dll"
      "${SYCL_DIR}/pi_level_zero.dll"
      "${SYCL_DIR}/pi_win_proxy_loader.dll"
      "${SYCL_DIR}/win_proxy_loader.dll"
    )
  else()
    file(GLOB SYCL_LIB LIST_DIRECTORIES FALSE
      "${SYCL_DIR}/libsycl.so.?"
      "${SYCL_DIR}/libpi_level_zero.so"
    )
  endif()
  list(APPEND DEPENDENT_LIBS ${SYCL_LIB})
endif()

if (WIN32)
  set(INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
else()
  set(INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

if (OSPRAY_SIGN_FILE)
  add_custom_target(sign_files
    COMMAND ${OSPRAY_SIGN_FILE} ${OSPRAY_SIGN_FILE_ARGS} ${DEPENDENT_LIBS} ${DEPENDENT_LIBS_DEBUG}
    COMMENT "Signing files"
    VERBATIM
  )
else()
  add_custom_target(sign_files COMMENT "Not signing files")
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(DEPENDENT_LIBS ${DEPENDENT_LIBS_DEBUG})
endif()
foreach(LIB ${DEPENDENT_LIBS})
  install(CODE
    "file(INSTALL \"${LIB}\" DESTINATION \${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR} FOLLOW_SYMLINK_CHAIN)"
    COMPONENT redist
  )
endforeach()

# Install MSVC runtime
if (WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  include(InstallRequiredSystemLibraries)
  list(FILTER CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS INCLUDE REGEX
      ".*msvcp[0-9]+\.dll|.*vcruntime[0-9]+\.dll|.*vcruntime[0-9]+_[0-9]+\.dll")
  install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
endif()
