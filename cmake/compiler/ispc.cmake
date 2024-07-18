## Copyright 2024 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

###############################################################################
## Generic ISPC macros/options ################################################
###############################################################################

set(ISPC_INCLUDE_DIR "")
macro (include_directories_ispc)
  list(APPEND ISPC_INCLUDE_DIR ${ARGN})
endmacro ()

set(ISPC_DEFINITIONS "")
macro (add_definitions_ispc)
  list(APPEND ISPC_DEFINITIONS ${ARGN})
endmacro ()

###############################################################################
## CPU specific macros/options ################################################
###############################################################################

## Find ISPC ##
find_program(ISPC_EXECUTABLE ispc HINTS ${ISPC_DIR_HINT} DOC "Path to the ISPC executable.")
if (NOT ISPC_EXECUTABLE)
  message(FATAL_ERROR "Could not find ISPC. Exiting.")
else()
  # Execute "ispc --version" and parse the version
  execute_process(COMMAND ${ISPC_EXECUTABLE} "--version"
                  OUTPUT_VARIABLE ISPC_INFO)
  string(REGEX MATCH "(.*), ([0-9]*\.[0-9]*\.[0-9]*[a-z]*) (.*)" _ ${ISPC_INFO})
  set(ISPC_VERSION ${CMAKE_MATCH_2})
  message(STATUS "Found ISPC v${ISPC_VERSION}: ${ISPC_EXECUTABLE}")
  # Execute "ispc --help" and parse supported archs
  execute_process(COMMAND ${ISPC_EXECUTABLE} "--help"
                  OUTPUT_VARIABLE ISPC_HELP)
  string(REGEX MATCH "--arch={((([a-z,0-9,-])+, |([a-z,0-9,-])+)+)}" _ ${ISPC_HELP})
  set(ISPC_ARCHS ${CMAKE_MATCH_1})
  if ("${ISPC_ARCHS}" STREQUAL "")
    message(WARNING "Can't determine ISPC supported architectures.")
  else()
    message(STATUS "ISPC supports: ${ISPC_ARCHS}")
    string(REPLACE ", " ";" ISPC_ARCHS_LIST ${ISPC_ARCHS})
  endif()
endif()

## ISPC config options ##

set(ISPC_ADDRESSING 32 CACHE STRING "32 vs 64 bit addressing in ispc")
set_property(CACHE ISPC_ADDRESSING PROPERTY STRINGS 32 64)
mark_as_advanced(ISPC_ADDRESSING)

macro(define_ispc_supported_arch ARCH_NAME ARCH_FILTER)
  set(ISPC_ARCHS_${ARCH_NAME}_LIST ${ISPC_ARCHS_LIST})
  list(FILTER ISPC_ARCHS_${ARCH_NAME}_LIST INCLUDE REGEX ${ARCH_FILTER})
  list(LENGTH ISPC_ARCHS_${ARCH_NAME}_LIST ARCH_LENGTH)
  if (${ARCH_LENGTH} GREATER 0)
    set(ISPC_${ARCH_NAME}_ENABLED TRUE)
  endif()
endmacro()

define_ispc_supported_arch(X86 "x86|x86-64")
define_ispc_supported_arch(ARM "arm|aarch64")
define_ispc_supported_arch(XE "xe64")

macro(define_ispc_isa_options ISA_NAME)
  set(ISPC_TARGET_${ISA_NAME} ${ARGV1} CACHE STRING "ispc target used for ${ISA_NAME} ISA")
  set_property(CACHE ISPC_TARGET_${ISA_NAME} PROPERTY STRINGS ${ARGN} NONE)
  #mark_as_advanced(ISPC_TARGET_${ISA_NAME})
endmacro()

if (ISPC_X86_ENABLED)
  define_ispc_isa_options(SSE4 sse4-i32x4 sse4-i32x8 sse4-i16x8 sse4-i8x16)
  define_ispc_isa_options(AVX avx1-i32x8 avx1-i32x4 avx1-i32x16 avx1-i64x4)
  define_ispc_isa_options(AVX2 avx2-i32x8 avx2-i32x4 avx2-i32x16 avx2-i64x4)
  define_ispc_isa_options(AVX512KNL avx512knl-x16)
  define_ispc_isa_options(AVX512SKX avx512skx-x16 avx512skx-x8)
  define_ispc_isa_options(AVX512SPR avx512spr-x16 avx512spr-x8)
endif()

macro(append_ispc_target_list ISA_NAME)
  set(_TARGET_NAME ISPC_TARGET_${ISA_NAME})
  if (NOT ${_TARGET_NAME} STREQUAL "NONE")
    list(APPEND ISPC_TARGET_LIST ${${_TARGET_NAME}})
  endif()
  unset(_TARGET_NAME)
endmacro()

unset(ISPC_TARGET_LIST)
if (ISPC_X86_ENABLED)
  append_ispc_target_list(SSE4)
  append_ispc_target_list(AVX)
  append_ispc_target_list(AVX2)
  append_ispc_target_list(AVX512KNL)
  append_ispc_target_list(AVX512SKX)
  if (NOT APPLE)
    append_ispc_target_list(AVX512SPR)
  endif()
endif()

## Macros ##

macro (ispc_read_dependencies ISPC_DEPENDENCIES_FILE)
  set(ISPC_DEPENDENCIES "")
  if (EXISTS ${ISPC_DEPENDENCIES_FILE})
    file(READ ${ISPC_DEPENDENCIES_FILE} contents)
    string(REPLACE " " ";"     contents "${contents}")
    string(REPLACE ";" "\\\\;" contents "${contents}")
    string(REPLACE "\n" ";"    contents "${contents}")
    foreach(dep ${contents})
      if (EXISTS ${dep})
        set(ISPC_DEPENDENCIES ${ISPC_DEPENDENCIES} ${dep})
      endif (EXISTS ${dep})
    endforeach(dep ${contents})
  endif ()
endmacro()

macro (ispc_compile)
  cmake_parse_arguments(ISPC_COMPILE "" "TARGET" "" ${ARGN})

  set(ISPC_ADDITIONAL_ARGS "")
  if (OSPRAY_STRICT_BUILD)
    list(APPEND ISPC_ADDITIONAL_ARGS --wno-perf)
  else()
    list(APPEND ISPC_ADDITIONAL_ARGS --woff)
  endif()
  # Check if CPU target is passed externally
  if (NOT ISPC_TARGET_CPU)
    set(ISPC_TARGETS ${ISPC_TARGET_LIST})
  else()
    set(ISPC_TARGETS ${ISPC_TARGET_CPU})
  endif()

  set(ISPC_TARGET_EXT ${CMAKE_CXX_OUTPUT_EXTENSION})
  string(REPLACE ";" "," ISPC_TARGET_ARGS "${ISPC_TARGETS}")

  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
      set(ISPC_ARCHITECTURE "aarch64")
    else()
      set(ISPC_ARCHITECTURE "x86-64")
    endif()
  else()
    set(ISPC_ARCHITECTURE "x86")
  endif()

  file(RELATIVE_PATH ISPC_TARGET_DIR_RELATIVE ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  set(ISPC_TARGET_DIR ${CMAKE_BINARY_DIR}/${ISPC_TARGET_DIR_RELATIVE})
  include_directories(${ISPC_TARGET_DIR})

  if(ISPC_INCLUDE_DIR)
    string(REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
    set(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS})
  endif()

  if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
  endif()
  #CAUTION: -O0/1 -g with ispc seg faults
  set(ISPC_FLAGS_DEBUG "-g" CACHE STRING "ISPC Debug flags")
  mark_as_advanced(ISPC_FLAGS_DEBUG)
  set(ISPC_FLAGS_RELEASE "-O3" CACHE STRING "ISPC Release flags")
  mark_as_advanced(ISPC_FLAGS_RELEASE)
  set(ISPC_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "ISPC Release with Debug symbols flags")
  mark_as_advanced(ISPC_FLAGS_RELWITHDEBINFO)
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELEASE})
  elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_DEBUG})
  elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELWITHDEBINFO})
  else ()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE (${CMAKE_BUILD_TYPE}) allows only the following values: Debug;Release;RelWithDebInfo")
  endif()

  # turn space separated list into ';' separated list
  string(REPLACE " " ";" ISPC_OPT_FLAGS "${ISPC_OPT_FLAGS}")

  if (NOT WIN32)
    list(APPEND ISPC_ADDITIONAL_ARGS --pic)
  endif()

  if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    list(APPEND ISPC_ADDITIONAL_ARGS --opt=disable-assertions)
  endif()

  # Also set the target-local include directories and defines if
  # we were given a target name
  set(ISPC_INCLUDE_DIRS_EXPR "")
  set(ISPC_COMPILE_DEFINITIONS_EXPR "")
  if (NOT "${ISPC_COMPILE_TARGET}" STREQUAL "")
    set(ISPC_INCLUDE_DIRS_EXPR
      "$<TARGET_PROPERTY:${ISPC_COMPILE_TARGET},INCLUDE_DIRECTORIES>")
    set(ISPC_COMPILE_DEFINITIONS_EXPR
      "$<TARGET_PROPERTY:${ISPC_COMPILE_TARGET},COMPILE_DEFINITIONS>")
  endif()

  set(ISPC_OBJECTS "")
  foreach(src ${ISPC_COMPILE_UNPARSED_ARGUMENTS})
    get_filename_component(fname ${src} NAME_WE)
    # The src_relpath will usually be the same as getting the DIRECTORY
    # component of the src file name, but we go through the full path
    # computation path to handle cases where the src name is an absolute
    # file path in some other location, where we need to compute a path to
    get_filename_component(src_dir ${src} ABSOLUTE)
    file(RELATIVE_PATH src_relpath ${CMAKE_CURRENT_LIST_DIR} ${src_dir})
    get_filename_component(src_relpath ${src_relpath} DIRECTORY)
    # Remove any relative paths up from the relative path
    string(REPLACE "../" "_/" src_relpath "${src_relpath}")

    set(outdir "${ISPC_TARGET_DIR}/${src_relpath}")
    set(outdir_relative "${ISPC_TARGET_DIR_RELATIVE}/${src_relpath}")
    set(input ${CMAKE_CURRENT_SOURCE_DIR}/${src})

    set(ISPC_DEPENDENCIES_FILE ${outdir}/${fname}.dev.idep)
    ispc_read_dependencies(${ISPC_DEPENDENCIES_FILE})

    set(results "${outdir}/${fname}.dev${ISPC_TARGET_EXT}")
    # if we have multiple targets add additional object files
    list(LENGTH ISPC_TARGETS NUM_TARGETS)
    if (NUM_TARGETS GREATER 1)
      foreach(target ${ISPC_TARGETS})
        string(REGEX REPLACE "-(i(8|16|32|64))?x(4|8|16|32|64)" "" target ${target})
        string(REPLACE "avx1" "avx" target ${target})
        list(APPEND results "${outdir}/${fname}.dev_${target}${ISPC_TARGET_EXT}")
      endforeach()
    endif()

    # use relative path for generated header file, because the path is
    # included as comment and the header will be distributed
    set(headerfile "${outdir}/${fname}_ispc.h")
    file(RELATIVE_PATH headerfile ${CMAKE_CURRENT_BINARY_DIR} ${headerfile})

    add_custom_command(
      OUTPUT ${results} ${outdir}/${fname}_ispc.h
      COMMAND ${CMAKE_COMMAND} -E make_directory ${outdir}
      COMMAND ${ISPC_EXECUTABLE}
        ${ISPC_DEFINITIONS}
        -I ${CMAKE_CURRENT_SOURCE_DIR}
        "$<$<BOOL:${ISPC_INCLUDE_DIRS_EXPR}>:-I$<JOIN:${ISPC_INCLUDE_DIRS_EXPR},;-I>>"
        ${ISPC_INCLUDE_DIR_PARMS}
        "$<$<BOOL:${ISPC_COMPILE_DEFINITIONS_EXPR}>:-D$<JOIN:${ISPC_COMPILE_DEFINITIONS_EXPR},;-D>>"
        --arch=${ISPC_ARCHITECTURE}
        --addressing=${ISPC_ADDRESSING}
        ${ISPC_OPT_FLAGS}
        --target=${ISPC_TARGET_ARGS}
        --opt=fast-math
        ${ISPC_ADDITIONAL_ARGS}
        -h ${headerfile}
        -MMM ${ISPC_DEPENDENCIES_FILE}
        -o ${outdir}/${fname}.dev${ISPC_TARGET_EXT}
        ${input}
      DEPENDS ${input} ${ISPC_DEPENDENCIES}
      COMMENT "Building ISPC object ${outdir_relative}/${fname}.dev${ISPC_TARGET_EXT}"
      COMMAND_EXPAND_LISTS
      VERBATIM
    )
    list(APPEND ISPC_OBJECTS ${results})

    # avoid race conditions if multiple targets compile the same ispc source file
    string(REPLACE "/" "_" dep_target "compile__${src}")
    if (NOT TARGET ${dep_target})
      add_custom_target(${dep_target} DEPENDS ${results})
    endif()
    list(APPEND ISPC_DEPENDENCY_TARGETS ${dep_target})
  endforeach()
endmacro()

###############################################################################
## Generic kernel compilation #################################################
###############################################################################

# Kept for backwards compatibility with existing CPU projects
function(ispc_target_add_sources name)
  ## Split-out C/C++ from ISPC files ##

  set(ISPC_SOURCES "")
  get_property(TARGET_SOURCES TARGET ${name} PROPERTY SOURCES)

  foreach(src ${ARGN})
    get_filename_component(ext ${src} EXT)
    if (ext STREQUAL ".ispc")
      list(APPEND ISPC_SOURCES ${src})
    else()
      list(APPEND TARGET_SOURCES ${src})
    endif()
  endforeach()

  if (ISPC_SOURCES)
    ispc_compile(${ISPC_SOURCES})
    list(APPEND TARGET_SOURCES ${ISPC_OBJECTS})
    add_dependencies(${name} ${ISPC_DEPENDENCY_TARGETS})
  endif()

  set_target_properties(${name} PROPERTIES SOURCES "${TARGET_SOURCES}")
endfunction()

