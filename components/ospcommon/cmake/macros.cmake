## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

## Macro for printing CMake variables ##
macro(print var)
  message("${var} = ${${var}}")
endmacro()

## Macro to print a warning message that only appears once ##
macro(ospray_warn_once IDENTIFIER MESSAGE)
  set(INTERNAL_WARNING "OSPRAY_WARNED_${IDENTIFIER}")
  if(NOT ${INTERNAL_WARNING})
    message(WARNING ${MESSAGE})
    set(${INTERNAL_WARNING} ON CACHE INTERNAL "Warned about '${MESSAGE}'")
  endif()
endmacro()


# workaround link issues to Embree ISPC exports
# ISPC only adds the ISA suffix during name mangling (and dynamic dispatch
# code) when compiling for multiple targets. Thus, when only one OSPRay ISA is
# selected, but Embree was compiled for multiple ISAs, we need to add a
# second, different, supported dummy target.
macro(ospray_fix_ispc_target_list)
  list(LENGTH OSPRAY_ISPC_TARGET_LIST NUM_TARGETS)
  if (NUM_TARGETS EQUAL 1)
    if (EMBREE_ISA_SUPPORTS_SSE2)
      list(APPEND OSPRAY_ISPC_TARGET_LIST sse2)
    elseif (EMBREE_ISA_SUPPORTS_SSE4 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "sse4")
      list(APPEND OSPRAY_ISPC_TARGET_LIST sse4)
    elseif (EMBREE_ISA_SUPPORTS_AVX AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx)
    elseif (EMBREE_ISA_SUPPORTS_AVX2 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx2")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx2)
    elseif (EMBREE_ISA_SUPPORTS_AVX512KNL AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx512knl-i32x16")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)
    elseif (EMBREE_ISA_SUPPORTS_AVX512SKX AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx512skx-i32x16")
      list(APPEND OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)
    endif()
  endif()
endmacro()

## Macro configure ISA targets for ispc ##
macro(ospray_configure_ispc_isa)

  set(OSPRAY_BUILD_ISA "ALL" CACHE STRING
      "Target ISA (SSE4, AVX, AVX2, AVX512KNL, AVX512SKX, or ALL)")
  string(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

  option(OSPRAY_BUILD_ISA_SCALAR
         "Include 'SCALAR' target (WARNING: may not work!)" OFF)
  mark_as_advanced(OSPRAY_BUILD_ISA_SCALAR)

  if (OSPRAY_BUILD_ISA_SCALAR)
    set(OSPRAY_SUPPORTED_ISAS SCALAR)
  endif()

  if(EMBREE_ISA_SUPPORTS_SSE4)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} SSE4)
  endif()
  if(EMBREE_ISA_SUPPORTS_AVX)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX)
  endif()
  if(EMBREE_ISA_SUPPORTS_AVX2)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX2)
  endif()
  if(EMBREE_ISA_SUPPORTS_AVX512KNL)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512KNL)
  endif()
  if(EMBREE_ISA_SUPPORTS_AVX512SKX)
    set(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512SKX)
  endif()

  set_property(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS
               ALL ${OSPRAY_SUPPORTED_ISAS})

  unset(OSPRAY_ISPC_TARGET_LIST)
  if (OSPRAY_BUILD_ISA STREQUAL "ALL")

    if(EMBREE_ISA_SUPPORTS_SSE4)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} sse4)
      message(STATUS "OSPRay SSE4 ISA target enabled.")
    endif()
    if(EMBREE_ISA_SUPPORTS_AVX)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx)
      message(STATUS "OSPRay AVX ISA target enabled.")
    endif()
    if(EMBREE_ISA_SUPPORTS_AVX2)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx2)
      message(STATUS "OSPRay AVX2 ISA target enabled.")
    endif()
    if(EMBREE_ISA_SUPPORTS_AVX512KNL)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512knl-i32x16)
      message(STATUS "OSPRay AVX512KNL ISA target enabled.")
    endif()
    if(EMBREE_ISA_SUPPORTS_AVX512SKX)
      set(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512skx-i32x16)
      message(STATUS "OSPRay AVX512SKX ISA target enabled.")
    endif()

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX512SKX")

    if(NOT EMBREE_ISA_SUPPORTS_AVX512SKX)
      message(FATAL_ERROR "Your Embree build does not support AVX512SKX!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX512KNL")

    if(NOT EMBREE_ISA_SUPPORTS_AVX512KNL)
      message(FATAL_ERROR "Your Embree build does not support AVX512KNL!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX2")

    if(NOT EMBREE_ISA_SUPPORTS_AVX2)
      message(FATAL_ERROR "Your Embree build does not support AVX2!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx2)

  elseif (OSPRAY_BUILD_ISA STREQUAL "AVX")

    if(NOT EMBREE_ISA_SUPPORTS_AVX)
      message(FATAL_ERROR "Your Embree build does not support AVX!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST avx)

  elseif (OSPRAY_BUILD_ISA STREQUAL "SSE4")

    if(NOT EMBREE_ISA_SUPPORTS_SSE4)
      message(FATAL_ERROR "Your Embree build does not support SSE4!")
    endif()
    set(OSPRAY_ISPC_TARGET_LIST sse4)

  else()
    message(ERROR "Invalid OSPRAY_BUILD_ISA value. "
                  "Please select one of ${OSPRAY_SUPPORTED_ISAS}, or ALL.")
  endif()

  ospray_fix_ispc_target_list()
endmacro()

## Target creation macros ##

macro(ospray_add_library name type)
  set(ISPC_SOURCES "")
  set(OTHER_SOURCES "")
  foreach(src ${ARGN})
    get_filename_component(ext ${src} EXT)
    if (ext STREQUAL ".ispc")
      set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    else()
      set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    endif ()
  endforeach()
  ospray_ispc_compile(${ISPC_SOURCES})
  add_library(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})
endmacro()

## Target install macros for OSPRay libraries ##
include(GNUInstallDirs)

macro(ospray_install_library name component export)

  if("${export}" STREQUAL "TRUE")
    set(_export_args EXPORT ${name}-targets)
  endif()

  install(TARGETS ${name} ${_export_args}
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

  # Install the namelink in the devel component. This command also includes the
  # RUNTIME and ARCHIVE components a second time to prevent an "install TARGETS
  # given no ARCHIVE DESTINATION for static library target" error. Installing
  # these components twice doesn't hurt anything.
  # Note (Qi): I am not sure if I need to add export target here again for them
  install(TARGETS ${name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
      NAMELINK_ONLY
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${component}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
  )

  if ("${export}" STREQUAL "TRUE")
    # Export targets to install directory
    # Note (Qi): this creates one file per target, thus we have to include all
    #   of them when compiling ospray related applications
    # --> changed from:
    # SET(OSPRAY_TARGET_LIST ${OSPRAY_TARGET_LIST} ${name}
    #     CACHE INTERNAL "" FORCE)
    set_property(GLOBAL APPEND PROPERTY OSPRAY_TARGETS_EXPORT ${name})
    install(EXPORT ${name}-targets
      FILE ospray-${name}-targets.cmake
      NAMESPACE ospray::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ospray-${OSPRAY_VERSION}
    )
  endif()

endmacro()

## Target versioning macro ##

macro(ospray_set_library_version name)
  set_target_properties(${name}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
endmacro()

# Helper function to return arguments of ospray_create_ in separate
# variables, prefixed with PREFIX
function(ospray_split_create_args PREFIX)
  set(MY_SOURCES "")
  set(LIBS "")
  set(MY_EXCLUDE_FROM_ALL FALSE)
  set(MY_EXPORT           FALSE)
  set(MY_COMPONENT ${OSPRAY_DEFAULT_COMPONENT})
  set(CURRENT_LIST MY_SOURCES)

  foreach(arg ${ARGN})
    if ("${arg}" STREQUAL "LINK")
      set(CURRENT_LIST LIBS)
    elseif ("${arg}" STREQUAL "EXCLUDE_FROM_ALL")
      set(MY_EXCLUDE_FROM_ALL TRUE)
    elseif ("${arg}" STREQUAL "EXPORT")
      set(MY_EXPORT TRUE)
    elseif ("${arg}" STREQUAL "COMPONENT")
      set(CURRENT_LIST MY_COMPONENT)
    else()
      list(APPEND ${CURRENT_LIST} ${arg})
    endif ()
  endforeach()

  # COMPONENT only required when installed
  if (NOT ${MY_EXCLUDE_FROM_ALL})
    list(LENGTH MY_COMPONENT COMPONENTS)
    if (COMPONENTS EQUAL 0)
      message(STATUS "No COMPONENT for installation specified! Using default COMPONENT name \"${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}\"")
      set(MY_COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
    else()
      list(GET MY_COMPONENT -1 MY_COMPONENT)
    endif()
  endif()

  set(${PREFIX}_SOURCES ${MY_SOURCES} PARENT_SCOPE)
  set(${PREFIX}_LIBS ${LIBS} PARENT_SCOPE)
  set(${PREFIX}_EXCLUDE_FROM_ALL ${MY_EXCLUDE_FROM_ALL} PARENT_SCOPE)
  set(${PREFIX}_EXPORT ${MY_EXPORT} PARENT_SCOPE)
  set(${PREFIX}_COMPONENT ${MY_COMPONENT} PARENT_SCOPE)
endfunction()

## Conveniance macro for creating OSPRay libraries ##
# Usage
#
#   ospray_create_library(<name> source1 [source2 ...]
#                         [LINK lib1 [lib2 ...]]
#                         [EXCLUDE_FROM_ALL]
#                         [COMPONENT component])
#
# will create and install shared library <name> from 'sources' with
# version OSPRAY_[SO]VERSION and optionally link against 'libs'.
# When EXCLUDE_FROM_ALL is given the library will not be installed and
# only be build when it is an explicit dependency.
# Per default the application will be installed in component
# OSPRAY_DEFAULT_COMPONENT which can be overridden individually with
# COMPONENT.

macro(ospray_create_library LIBRARY_NAME)
  ospray_split_create_args(LIBRARY ${ARGN})

  ospray_add_library(${LIBRARY_NAME} SHARED ${LIBRARY_SOURCES})
  target_link_libraries(${LIBRARY_NAME} PUBLIC ${LIBRARY_LIBS})
  ospray_set_library_version(${LIBRARY_NAME})
  if(${LIBRARY_EXCLUDE_FROM_ALL})
    set_target_properties(${LIBRARY_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  else()
    if(${LIBRARY_EXPORT})
      ospray_install_library(${LIBRARY_NAME} ${LIBRARY_COMPONENT} TRUE)
    else()
      ospray_install_library(${LIBRARY_NAME} ${LIBRARY_COMPONENT} FALSE)
    endif()
  endif()
endmacro()

## Conveniance macro for creating OSPRay applications ##
# Usage
#
#   ospray_create_application(<name> source1 [source2 ...]
#                             [LINK lib1 [lib2 ...]]
#                             [EXCLUDE_FROM_ALL]
#                             [COMPONENT component])
#
# will create and install application <name> from 'sources' with version
# OSPRAY_VERSION and optionally link against 'libs'.
# When EXCLUDE_FROM_ALL is given the application will not be installed
# and only be build when it is an explicit dependency.
# Per default the application will be installed in component
# OSPRAY_DEFAULT_COMPONENT which can be overridden individually with
# COMPONENT.

macro(ospray_create_application APP_NAME)
  ospray_split_create_args(APP ${ARGN})

  add_executable(${APP_NAME} ${APP_SOURCES})
  target_link_libraries(${APP_NAME} ${APP_LIBS})
  if (WIN32)
    set_target_properties(${APP_NAME} PROPERTIES VERSION ${OSPRAY_VERSION})
  endif()
  if(${APP_EXCLUDE_FROM_ALL})
    set_target_properties(${APP_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  else()
    install(TARGETS ${APP_NAME}
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${APP_COMPONENT}
    )
  endif()
endmacro()

## Conveniance macro for creating OSPRay test applications ##
# Usage - same as ospray_create_application
#
# Will build the app if 'OSPRAY_ENABLE_TESTING=ON' in CMake

macro(ospray_create_test)
  if (OSPRAY_ENABLE_TESTING)
    ospray_create_application(
      ${ARGN}
      COMPONENT test
    )
  endif()
endmacro()

## Conveniance macro for installing OSPRay headers ##
# Usage
#
#   ospray_install_sdk_headers(header1 [header2 ...] [DESTINATION destination])
#
# will install headers into ${CMAKE_INSTALL_PREFIX}/ospray/SDK/${destination},
# where destination is optional.

macro(ospray_install_sdk_headers)
  set(HEADERS "")
  set(MY_DESTINATION "")

  set(CURRENT_LIST HEADERS)
  foreach(arg ${ARGN})
    if ("${arg}" STREQUAL "DESTINATION")
      set(CURRENT_LIST MY_DESTINATION)
    else()
      list(APPEND ${CURRENT_LIST} ${arg})
    endif ()
  endforeach()

  install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/${MY_DESTINATION}
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

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(OSPRAY_COMPILER_ICC TRUE)
    if(WIN32) # icc on Windows behaves like msvc
      include(msvc)
    else()
      include(icc)
    endif()
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(OSPRAY_COMPILER_GCC TRUE)
    include(gcc)
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(OSPRAY_COMPILER_CLANG TRUE)
    include(clang)
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(OSPRAY_COMPILER_MSVC TRUE)
    include(msvc)
  else()
    message(FATAL_ERROR
            "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
  endif()

  if (WIN32)
    # increase stack to 8MB (the default size of 1MB is too small for our apps)
    # note: linker options are independent of compiler (icc or MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
  endif()
endmacro()

macro(ospray_disable_compiler_warnings)
  if (NOT OSPRAY_COMPILER_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  #elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
  endif()
endmacro()

## Tasking system configuration macro ##

macro(ospray_configure_tasking_system)
  # -------------------------------------------------------
  # Setup tasking system build configuration
  # -------------------------------------------------------

  # NOTE(jda) - Notice that this implies that OSPRAY_CONFIGURE_COMPILER() has
  #             been called before this macro!
  if(OSPRAY_COMPILER_ICC)
    set(CILK_STRING "Cilk")
  endif()

  set(OSPRAY_TASKING_SYSTEM TBB CACHE STRING
      "Per-node thread tasking system [TBB,OpenMP,Cilk,LibDispatch,Internal,Debug]")

  set_property(CACHE OSPRAY_TASKING_SYSTEM PROPERTY
               STRINGS TBB ${CILK_STRING} OpenMP Internal LibDispatch Debug)
  mark_as_advanced(OSPRAY_TASKING_SYSTEM)

  # NOTE(jda) - Make the OSPRAY_TASKING_SYSTEM build option case-insensitive
  string(TOUPPER ${OSPRAY_TASKING_SYSTEM} OSPRAY_TASKING_SYSTEM_ID)

  set(OSPRAY_TASKING_TBB         FALSE)
  set(OSPRAY_TASKING_CILK        FALSE)
  set(OSPRAY_TASKING_OPENMP      FALSE)
  set(OSPRAY_TASKING_INTERNAL    FALSE)
  set(OSPRAY_TASKING_LIBDISPATCH FALSE)
  set(OSPRAY_TASKING_DEBUG       FALSE)

  if(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "TBB")
    set(OSPRAY_TASKING_TBB TRUE)
  elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "CILK")
    set(OSPRAY_TASKING_CILK TRUE)
  elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "OPENMP")
    set(OSPRAY_TASKING_OPENMP TRUE)
  elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "INTERNAL")
    set(OSPRAY_TASKING_INTERNAL TRUE)
  elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "LIBDISPATCH")
    set(OSPRAY_TASKING_LIBDISPATCH TRUE)
  else()
    set(OSPRAY_TASKING_DEBUG TRUE)
  endif()

  unset(TASKING_SYSTEM_LIBS)

  if(OSPRAY_TASKING_TBB)
    find_package(TBB REQUIRED)
    add_definitions(-DOSPRAY_TASKING_TBB)
    include_directories(${TBB_INCLUDE_DIRS})
    set(TASKING_SYSTEM_LIBS ${TBB_LIBRARIES})
  else(OSPRAY_TASKING_TBB)
    unset(TBB_INCLUDE_DIR          CACHE)
    unset(TBB_LIBRARY              CACHE)
    unset(TBB_LIBRARY_DEBUG        CACHE)
    unset(TBB_LIBRARY_MALLOC       CACHE)
    unset(TBB_LIBRARY_MALLOC_DEBUG CACHE)
    if(OSPRAY_TASKING_OPENMP)
      find_package(OpenMP)
      if (OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        if(OSPRAY_COMPILER_ICC) # workaround linker issue #115
          set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -liomp5")
        endif()
        add_definitions(-DOSPRAY_TASKING_OMP)
      endif()
    elseif(OSPRAY_TASKING_CILK)
      add_definitions(-DOSPRAY_TASKING_CILK)
      if (OSPRAY_COMPILER_GCC OR OSPRAY_COMPILER_CLANG)
        ospray_warn_once(UNSAFE_USE_OF_CILK
            "You are using Cilk with GCC or Clang...use at your own risk!")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcilkplus")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus")
      endif()
    elseif(OSPRAY_TASKING_INTERNAL)
      add_definitions(-DOSPRAY_TASKING_INTERNAL)
    elseif(OSPRAY_TASKING_LIBDISPATCH)
      find_package(libdispatch REQUIRED)
      include_directories(${LIBDISPATCH_INCLUDE_DIRS})
      set(TASKING_SYSTEM_LIBS ${LIBDISPATCH_LIBRARIES})
      add_definitions(-DOSPRAY_TASKING_LIBDISPATCH)
    else()#Debug
      # Do nothing, will fall back to scalar code (useful for debugging)
    endif()
  endif(OSPRAY_TASKING_TBB)
endmacro()

## MPI configuration macro ##

macro(OSPRAY_CONFIGURE_MPI)
  if (WIN32) # FindMPI does not find Intel MPI on Windows, we need to help here
    find_package(MPI)

    # need to strip quotes, otherwise CMake treats it as relative path
    string(REGEX REPLACE "^\"|\"$" "" MPI_CXX_INCLUDE_PATH ${MPI_CXX_INCLUDE_PATH})

    if (NOT MPI_CXX_FOUND)
      # try again, hinting the compiler wrappers
      set(MPI_CXX_COMPILER mpicxx.bat)
      set(MPI_C_COMPILER mpicc.bat)
      find_package(MPI)

      if (NOT MPI_CXX_LIBRARIES)
        set(MPI_LIB_PATH ${MPI_CXX_INCLUDE_PATH}\\..\\lib)

        set(MPI_LIB "MPI_LIB-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
        find_library(MPI_LIB NAMES impi HINTS ${MPI_LIB_PATH})
        set(MPI_C_LIB ${MPI_LIB})
        set(MPI_C_LIBRARIES ${MPI_LIB} CACHE STRING "MPI C libraries to link against" FORCE)

        set(MPI_LIB "MPI_LIB-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
        find_library(MPI_LIB NAMES impicxx HINTS ${MPI_LIB_PATH})
        set(MPI_CXX_LIBRARIES ${MPI_C_LIB} ${MPI_LIB} CACHE STRING "MPI CXX libraries to link against" FORCE)
        set(MPI_LIB "MPI_LIB-NOTFOUND" CACHE INTERNAL "Scratch variable for MPI lib detection" FORCE)
      endif()
    endif()
  else()
    find_package(MPI REQUIRED)
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")

  include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})
endmacro()

# Keep backwards compatible configure_mpi() macro, but warn of deprecation
macro(configure_mpi)
  ospray_warn_once(CONFIGURE_MPI_DEPRECATION
                   "configure_mpi() is deprecated, use new ospray_configure_mpi().")
  ospray_configure_mpi()
endmacro()
