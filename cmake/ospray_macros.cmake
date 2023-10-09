## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(CMakeFindDependencyMacro)

## Macro for printing CMake variables ##
macro(print var)
  message("${var} = ${${var}}")
endmacro()

## Get a list of subdirectories (single level) under a given directory
macro(get_subdirectories result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

## Get all subdirectories and call add_subdirectory() if it has a CMakeLists.txt
macro(add_all_subdirectories_except except)
  set(e ${except})
  file(GLOB dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
  foreach(dir ${dirs})
    if (NOT "${dir}X" STREQUAL "${except}X" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
      add_subdirectory(${dir})
    endif()
  endforeach()
endmacro()
macro(add_all_subdirectories)
  add_all_subdirectories_except("")
endmacro()

## Setup CMAKE_BUILD_TYPE to have a default + cycle between options in UI
macro(ospray_configure_build_type)
  set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  endif()
  if (WIN32)
    if (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
      set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
          CACHE STRING "List of generated configurations." FORCE)
      set(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
          CACHE INTERNAL "Default CMake configuration types set.")
    endif()
  else()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
  endif()

  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(OSPRAY_BUILD_RELEASE        TRUE )
    set(OSPRAY_BUILD_DEBUG          FALSE)
    set(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(OSPRAY_BUILD_RELEASE        FALSE)
    set(OSPRAY_BUILD_DEBUG          TRUE )
    set(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  else()
    set(OSPRAY_BUILD_RELEASE        FALSE)
    set(OSPRAY_BUILD_DEBUG          FALSE)
    set(OSPRAY_BUILD_RELWITHDEBINFO TRUE )
  endif()
endmacro()

# workaround link issues to Embree ISPC exports
# ISPC only adds the ISA suffix during name mangling (and dynamic dispatch
# code) when compiling for multiple targets. Thus, when only one OSPRay ISA is
# selected, but Embree was compiled for multiple ISAs, we need to add a
# second, different, supported dummy target.
macro(ospray_fix_ispc_target_list)
  list(LENGTH OSPRAY_ISPC_TARGET_LIST NUM_TARGETS)
  if (NUM_TARGETS EQUAL 1
      AND NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "neon-i32x4"
      AND NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "neon-i32x8")
    if (EMBREE_ISA_SSE42 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "sse4")
      list(APPEND OSPRAY_ISPC_TARGET_LIST sse4)
    elseif (EMBREE_ISA_AVX AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx)
    elseif (EMBREE_ISA_AVX2 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx2")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx2)
    elseif (EMBREE_ISA_AVX512 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx512skx-i32x16")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)
    elseif (EMBREE_ISA_NEON AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "neon-i32x4")
      list(APPEND OSPRAY_ISPC_TARGET_LIST neon-i32x4)
    elseif (EMBREE_ISA_NEON2X AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "neon-i32x8")
      list(APPEND OSPRAY_ISPC_TARGET_LIST neon-i32x8)
    endif()
  endif()
endmacro()

## Macro configure ISA targets for ispc ##
macro(ospray_configure_ispc_isa)

  set(OSPRAY_BUILD_ISA "ALL" CACHE STRING
    "Target ISA (SSE4, AVX, AVX2, AVX512SKX, NEON, NEON2X, or ALL)")
  string(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

  if(EMBREE_ISA_SSE42 AND OPENVKL_ISA_SSE4)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} SSE4)
  endif()
  if(EMBREE_ISA_AVX AND OPENVKL_ISA_AVX)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX)
  endif()
  if(EMBREE_ISA_AVX2 AND OPENVKL_ISA_AVX2)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX2)
  endif()
  if(EMBREE_ISA_AVX512 AND OPENVKL_ISA_AVX512SKX)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512SKX)
  endif()
  if(EMBREE_ISA_NEON AND OPENVKL_ISA_NEON)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} NEON)
  endif()
  if(EMBREE_ISA_NEON2X AND OPENVKL_ISA_NEON2X)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} NEON2X)
  endif()

  set_property(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS
               ALL ${OSPRAY_SUPPORTED_ISAS})

  unset(OSPRAY_ISPC_TARGET_LIST)
  if (OSPRAY_BUILD_ISA STREQUAL "ALL")

    if(EMBREE_ISA_SSE42 AND OPENVKL_ISA_SSE4)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} sse4)
      message(STATUS "OSPRay SSE4 ISA target enabled.")
    endif()
    if(EMBREE_ISA_AVX AND OPENVKL_ISA_AVX)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx)
      message(STATUS "OSPRay AVX ISA target enabled.")
    endif()
    if(EMBREE_ISA_AVX2 AND OPENVKL_ISA_AVX2)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx2)
      message(STATUS "OSPRay AVX2 ISA target enabled.")
    endif()
    if(EMBREE_ISA_AVX512 AND OPENVKL_ISA_AVX512SKX)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512skx-i32x16)
      message(STATUS "OSPRay AVX512SKX ISA target enabled.")
    endif()
    if(EMBREE_ISA_NEON AND OPENVKL_ISA_NEON)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} neon-i32x4)
      message(STATUS "OSPRay NEON ISA target enabled.")
    endif()
    if(EMBREE_ISA_NEON2X AND OPENVKL_ISA_NEON2X)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} neon-i32x8)
      message(STATUS "OSPRay NEON2X ISA target enabled.")
    endif()

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX512SKX")

    if(NOT EMBREE_ISA_AVX512)
      message(FATAL_ERROR "Your Embree build does not support AVX512SKX!")
    endif()
    if(NOT OPENVKL_ISA_AVX512SKX)
      message(FATAL_ERROR "Your Open VKL build does not support AVX512SKX!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX2")

    if(NOT EMBREE_ISA_AVX2)
      message(FATAL_ERROR "Your Embree build does not support AVX2!")
    endif()
    if(NOT OPENVKL_ISA_AVX2)
      message(FATAL_ERROR "Your Open VKL build does not support AVX2!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx2)

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX")

    if(NOT EMBREE_ISA_AVX)
      message(FATAL_ERROR "Your Embree build does not support AVX!")
    endif()
    if(NOT OPENVKL_ISA_AVX)
      message(FATAL_ERROR "Your Open VKL build does not support AVX!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx)

  elseif (OSPRAY_BUILD_ISA STREQUAL "SSE4")

    if(NOT EMBREE_ISA_SSE42)
      message(FATAL_ERROR "Your Embree build does not support SSE4!")
    endif()
    if(NOT OPENVKL_ISA_SSE4)
      message(FATAL_ERROR "Your Open VKL build does not support SSE4!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST sse4)

  elseif (OSPRAY_BUILD_ISA STREQUAL "NEON")

    if (NOT EMBREE_ISA_NEON)
      message(FATAL_ERROR "Your Embree build does not support NEON!")
    endif()
    if (NOT OPENVKL_ISA_NEON)
      message(FATAL_ERROR "Your OpenVKL build does not support NEON!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST neon-i32x4)

  elseif (OSPRAY_BUILD_ISA STREQUAL "NEON2X")

    if (NOT EMBREE_ISA_NEON2X)
      message(FATAL_ERROR "Your Embree build does not support NEON2X!")
    endif()
    if (NOT OPENVKL_ISA_NEON2X)
      message(FATAL_ERROR "Your OpenVKL build does not support NEON2X!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST neon-i32x8)

  else()
    message(FATAL_ERROR "Invalid OSPRAY_BUILD_ISA value. "
                  "Please select one of ${OSPRAY_SUPPORTED_ISAS}, or ALL.")
  endif()

  ospray_fix_ispc_target_list()
endmacro()

macro(ospray_add_sycl_target target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_compile_options(${target} PUBLIC ${OSPRAY_CXX_FLAGS_SYCL})
  target_link_options(${target} PUBLIC ${OSPRAY_LINKER_FLAGS_SYCL})
endmacro()

## Target creation macros ##

set(OSPRAY_SIGN_FILE_ARGS -q)
if (APPLE)
  list(APPEND OSPRAY_SIGN_FILE_ARGS -o runtime -e ${CMAKE_SOURCE_DIR}/scripts/release/ospray.entitlements)
endif()

macro(ospray_sign_target name)
  if (OSPRAY_SIGN_FILE)
    if (APPLE)
      # on OSX we strip manually before signing instead of setting CPACK_STRIP_FILES
      add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_STRIP} -x $<TARGET_FILE:${name}>
        COMMAND ${OSPRAY_SIGN_FILE} ${OSPRAY_SIGN_FILE_ARGS} $<TARGET_FILE:${name}>
        COMMENT "Stripping and signing target"
        VERBATIM
      )
    else()
      add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${OSPRAY_SIGN_FILE} ${OSPRAY_SIGN_FILE_ARGS} $<TARGET_FILE:${name}>
        COMMENT "Signing target"
        VERBATIM
      )
    endif()
  endif()
endmacro()

macro(ospray_install_library name component)
  set_target_properties(${name}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
  ospray_install_target(${name} ${component})
  ospray_sign_target(${name})

  if (${ARGN})
    # Install the namelink in the devel component. This command also includes the
    # RUNTIME and ARCHIVE components a second time to prevent an "install TARGETS
    # given no ARCHIVE DESTINATION for static library target" error. Installing
    # these components twice doesn't hurt anything.
    install(TARGETS ${name}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT devel
        NAMELINK_ONLY
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT ${component}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT devel
    )
  endif()
endmacro()

macro(ospray_install_target name component)
  install(TARGETS ${name}
    EXPORT ospray_Exports
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT ${component}
      NAMELINK_SKIP
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${component}
    # ... and the import lib into the devel package
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
  )

  install(EXPORT ospray_Exports
    DESTINATION ${OSPRAY_CMAKECONFIG_DIR}
    NAMESPACE ospray::
    COMPONENT devel
  )
endmacro()

## Compiler configuration macros ##

macro(ospray_configure_compiler)
  # unhide compiler to make it easier for users to see what they are using
  mark_as_advanced(CLEAR CMAKE_CXX_COMPILER)

  option(OSPRAY_STRICT_BUILD "Build with additional warning flags" ON)
  mark_as_advanced(OSPRAY_STRICT_BUILD)

  option(OSPRAY_WARN_AS_ERRORS "Treat warnings as errors" OFF)
  mark_as_advanced(OSPRAY_WARN_AS_ERRORS)

  set(OSPRAY_COMPILER_ICC   FALSE)
  set(OSPRAY_COMPILER_GCC   FALSE)
  set(OSPRAY_COMPILER_CLANG FALSE)
  set(OSPRAY_COMPILER_MSVC  FALSE)
  set(OSPRAY_COMPILER_DPCPP FALSE)

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM" OR OSPRAY_MODULE_GPU)
    set(OSPRAY_COMPILER_DPCPP TRUE)
    include(dpcpp)
    if(WIN32)
      set(CMAKE_NINJA_CMCLDEPS_RC OFF) # workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/18311
    endif()
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(OSPRAY_COMPILER_ICC TRUE)
    if(WIN32)
      set(CMAKE_NINJA_CMCLDEPS_RC OFF) # workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/18311
      include(msvc) # icc on Windows behaves like msvc
    else()
      include(icc)
    endif()
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(OSPRAY_COMPILER_GCC TRUE)
    include(gcc)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
          "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(OSPRAY_COMPILER_CLANG TRUE)
    include(clang)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(OSPRAY_COMPILER_MSVC TRUE)
    include(msvc)
  else()
    message(FATAL_ERROR
            "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
  endif()

  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

  if (WIN32)
    # increase stack to 8MB (the default size of 1MB is too small for our apps)
    if (OSPRAY_MODULE_GPU)
      get_filename_component(SYCL_COMPILER_NAME ${CMAKE_CXX_COMPILER} NAME_WE)
      if (SYCL_COMPILER_NAME STREQUAL "clang++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
      endif()
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
  endif()
endmacro()

macro(ospray_disable_compiler_warnings)
  if (NOT OSPRAY_COMPILER_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  endif()
endmacro()

## Embree functions/macros ##

function(ospray_check_embree_feature FEATURE VALUE DESC)
  set(FEATURE EMBREE_${FEATURE})
  if (${ARGN})
    if (${FEATURE} STREQUAL ${VALUE})
      return()
    endif()
  else()
    if ((${VALUE} AND ${FEATURE}) OR (NOT ${VALUE} AND NOT ${FEATURE}))
      return()
    endif()
  endif()
  message(FATAL_ERROR "OSPRay requires Embree to ${DESC} (compiled with ${FEATURE}=${VALUE}).")
endfunction()

function(ospray_verify_embree_features)
  ospray_check_embree_feature(ISPC_SUPPORT ON "support ISPC")
  ospray_check_embree_feature(FILTER_FUNCTION ON "support intersection filter")
  ospray_check_embree_feature(GEOMETRY_TRIANGLE ON "support triangle geometries")
  ospray_check_embree_feature(GEOMETRY_CURVE ON "support spline curve geometries")
  ospray_check_embree_feature(GEOMETRY_USER ON "support user geometries")
  ospray_check_embree_feature(RAY_PACKETS ON "support ray packets")
  ospray_check_embree_feature(BACKFACE_CULLING OFF "have backface culling disabled")
# FIXME available in Embree >v4.3
#  ospray_check_embree_feature(GEOMETRY_INSTANCE ON "support instances")
  ospray_check_embree_feature(MAX_INSTANCE_LEVEL_COUNT 1 "only support single-level instancing" ON)
  if (OSPRAY_MODULE_GPU)
    ospray_check_embree_feature(SYCL_SUPPORT ON "support DPC++/SYCL")
  endif()
endfunction()

macro(ospray_find_embree EMBREE_VERSION_REQUIRED FIND_AS_DEPENDENCY)
  if (${FIND_AS_DEPENDENCY})
    find_dependency(embree ${EMBREE_VERSION_REQUIRED})
  else()
    find_package(embree ${EMBREE_VERSION_REQUIRED})
  endif()
  if (NOT embree_FOUND)
    message(FATAL_ERROR
            "We did not find Embree installed on your system. OSPRay requires"
            " an Embree installation >= v${EMBREE_VERSION_REQUIRED}, please"
            " download and extract Embree (or compile Embree from source), then"
            " set the 'embree_DIR' variable to the installation (or build)"
            " directory.")
  endif()
  # Get Embree CPU info
  get_target_property(EMBREE_INCLUDE_DIRS embree
    INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(CONFIGURATIONS embree IMPORTED_CONFIGURATIONS)
  list(GET CONFIGURATIONS 0 CONFIGURATION)
  get_target_property(EMBREE_LIBRARY embree
      IMPORTED_LOCATION_${CONFIGURATION})
  # Get Embree SYCL info if DPCPP was enabled
  if (EMBREE_SYCL_SUPPORT)
    get_target_property(CONFIGURATIONS embree_sycl IMPORTED_CONFIGURATIONS)
    list(GET CONFIGURATIONS 0 CONFIGURATION)
    get_target_property(EMBREE_SYCL_LIBRARY embree_sycl
      IMPORTED_LOCATION_${CONFIGURATION})
  endif()
  message(STATUS "Found Embree v${embree_VERSION}: ${EMBREE_LIBRARY}")
endmacro()

macro(ospray_find_openvkl OPENVKL_VERSION_REQUIRED FIND_AS_DEPENDENCY)
  if (${FIND_AS_DEPENDENCY})
    find_dependency(openvkl ${OPENVKL_VERSION_REQUIRED})
  else()
    find_package(openvkl ${OPENVKL_VERSION_REQUIRED})
  endif()
  if (openvkl_FOUND)
    get_target_property(OPENVKL_INCLUDE_DIRS openvkl::openvkl
        INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(OPENVKL_CPU_DEVICE_INCLUDE_DIRS openvkl::openvkl_module_cpu_device
        INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(CONFIGURATIONS openvkl::openvkl IMPORTED_CONFIGURATIONS)
    list(GET CONFIGURATIONS 0 CONFIGURATION)
    get_target_property(OPENVKL_LIBRARY openvkl::openvkl
        IMPORTED_LOCATION_${CONFIGURATION})
    message(STATUS "Found Open VKL v${openvkl_VERSION}: ${OPENVKL_LIBRARY}")
  endif()
endmacro()
