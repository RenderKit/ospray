## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

## Tasking system target macros ##

macro(ospray_configure_tasking_system)
  # -------------------------------------------------------
  # Setup tasking system build configuration
  # -------------------------------------------------------

  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)

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
    find_package(TBB REQUIRED)
  else()
    unset(TBB_INCLUDE_DIR          CACHE)
    unset(TBB_LIBRARY              CACHE)
    unset(TBB_LIBRARY_DEBUG        CACHE)
    unset(TBB_LIBRARY_MALLOC       CACHE)
    unset(TBB_LIBRARY_MALLOC_DEBUG CACHE)
    if(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "CILK")
      set(OSPRAY_TASKING_CILK TRUE)
    elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "OPENMP")
      set(OSPRAY_TASKING_OPENMP TRUE)
      find_package(OpenMP)
    elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "INTERNAL")
      set(OSPRAY_TASKING_INTERNAL TRUE)
    elseif(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "LIBDISPATCH")
      set(OSPRAY_TASKING_LIBDISPATCH TRUE)
    else()
      set(OSPRAY_TASKING_DEBUG TRUE)
    endif()
  endif()

  set(OSPRAY_TASKING_LIBS ${CMAKE_THREAD_LIBS_INIT})

  if(OSPRAY_TASKING_TBB)
    set(OSPRAY_TASKING_DEFINITIONS -DOSPRAY_TASKING_TBB)
    set(OSPRAY_TASKING_INCLUDES ${TBB_INCLUDE_DIRS})
    set(OSPRAY_TASKING_LIBS ${OSPRAY_TASKING_LIBS} ${TBB_LIBRARIES})
  elseif(OSPRAY_TASKING_OPENMP)
    if (OPENMP_FOUND)
      set(OSPRAY_TASKING_OPTIONS "${OpenMP_CXX_FLAGS}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
      if(OSPRAY_COMPILER_ICC) # workaround linker issue #115
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -liomp5")
      endif()
      set(OSPRAY_TASKING_DEFINITIONS -DOSPRAY_TASKING_OMP)
    endif()
  elseif(OSPRAY_TASKING_CILK)
    set(OSPRAY_TASKING_DEFINITIONS -DOSPRAY_TASKING_CILK)
    if (OSPRAY_COMPILER_GCC OR OSPRAY_COMPILER_CLANG)
      ospray_warn_once(UNSAFE_USE_OF_CILK
          "You are using Cilk with GCC or Clang...use at your own risk!")
      set(OSPRAY_TASKING_OPTIONS "-fcilkplus")
    endif()
  elseif(OSPRAY_TASKING_INTERNAL)
    set(OSPRAY_TASKING_DEFINITIONS -DOSPRAY_TASKING_INTERNAL)
  elseif(OSPRAY_TASKING_LIBDISPATCH)
    find_package(libdispatch REQUIRED)
    set(OSPRAY_TASKING_INCLUDES ${LIBDISPATCH_INCLUDE_DIRS})
    set(OSPRAY_TASKING_LIBS ${OSPRAY_TASKING_LIBS} ${LIBDISPATCH_LIBRARIES})
    set(OSPRAY_TASKING_DEFINITIONS -DOSPRAY_TASKING_LIBDISPATCH)
  else()#Debug
    # Do nothing, will fall back to scalar code (useful for debugging)
  endif()
endmacro()

macro(ospray_create_tasking_target)
  add_library(ospray_tasking INTERFACE)

  target_include_directories(ospray_tasking
  INTERFACE
    ${OSPRAY_TASKING_INCLUDES}
  )

  target_link_libraries(ospray_tasking
  INTERFACE
    ${OSPRAY_TASKING_LIBS}
  )

  target_compile_definitions(ospray_tasking
  INTERFACE
    ${OSPRAY_TASKING_DEFINITIONS}
  )

  target_compile_options(ospray_tasking
  INTERFACE
    ${OSPRAY_TASKING_OPTIONS}
  )
endmacro()

## Embree functions/macros ##

function(ospray_check_embree_feature FEATURE DESCRIPTION)
  set(FEATURE EMBREE_${FEATURE})
  if(NOT ${ARGN})
    if (${FEATURE})
      message(FATAL_ERROR "OSPRay requires Embree to be compiled "
              "without ${DESCRIPTION} (${FEATURE}=OFF).")
    endif()
  else()
    if (NOT ${FEATURE})
      message(FATAL_ERROR "OSPRay requires Embree to be compiled "
              "with support for ${DESCRIPTION} (${FEATURE}=ON).")
    endif()
  endif()
endfunction()

function(ospray_verify_embree_features)
  ospray_check_embree_feature(ISPC_SUPPORT ISPC)
  ospray_check_embree_feature(FILTER_FUNCTION "intersection filter")
  ospray_check_embree_feature(GEOMETRY_TRIANGLE "triangle geometries")
  ospray_check_embree_feature(GEOMETRY_CURVE "spline curve geometries")
  ospray_check_embree_feature(GEOMETRY_USER "user geometries")
  ospray_check_embree_feature(RAY_PACKETS "ray packets")
  ospray_check_embree_feature(BACKFACE_CULLING "backface culling" OFF)
endfunction()

macro(ospray_create_embree_target)
  if (NOT TARGET embree::embree)
    add_library(embree INTERFACE)

    target_include_directories(embree
    INTERFACE
      $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
    )

    target_link_libraries(embree
    INTERFACE
      $<BUILD_INTERFACE:${EMBREE_LIBRARIES}>
    )
  endif()
endmacro()

macro(ospray_find_embree EMBREE_VERSION_REQUIRED)
  find_package(embree ${EMBREE_VERSION_REQUIRED} QUIET)
  if(NOT DEFINED EMBREE_INCLUDE_DIRS)
    message(FATAL_ERROR
            "We did not find Embree installed on your system. OSPRay requires"
            " an Embree installation >= v${EMBREE_VERSION_REQUIRED}, please"
            " download and extract Embree (or compile Embree from source), then"
            " set the 'embree_DIR' variable to the installation (or build)"
            " directory.")
  else()
    message(STATUS "Found Embree v${EMBREE_VERSION}: ${EMBREE_LIBRARY}")
  endif()

  set(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
endmacro()

macro(ospray_determine_embree_isa_support)
  if (EMBREE_MAX_ISA STREQUAL "DEFAULT" OR
      EMBREE_MAX_ISA STREQUAL "NONE")
    set(EMBREE_ISA_SUPPORTS_SSE2      ${EMBREE_ISA_SSE2})
    set(EMBREE_ISA_SUPPORTS_SSE4      ${EMBREE_ISA_SSE42})
    set(EMBREE_ISA_SUPPORTS_AVX       ${EMBREE_ISA_AVX})
    set(EMBREE_ISA_SUPPORTS_AVX2      ${EMBREE_ISA_AVX2})
    set(EMBREE_ISA_SUPPORTS_AVX512KNL ${EMBREE_ISA_AVX512KNL})
    set(EMBREE_ISA_SUPPORTS_AVX512SKX ${EMBREE_ISA_AVX512SKX})
  else()
    set(EMBREE_ISA_SUPPORTS_SSE2      FALSE)
    set(EMBREE_ISA_SUPPORTS_SSE4      FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX       FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX2      FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX512KNL FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX512SKX FALSE)

    if (EMBREE_MAX_ISA STREQUAL "SSE2")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
    elseif (EMBREE_MAX_ISA MATCHES "SSE4\\.[12]$")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX  TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX2")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX  TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2 TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX512KNL")
      set(EMBREE_ISA_SUPPORTS_SSE2      TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX       TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512KNL TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX512SKX")
      set(EMBREE_ISA_SUPPORTS_SSE2      TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX       TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512KNL TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512SKX TRUE)
    endif()
  endif()

  if (NOT (EMBREE_ISA_SUPPORTS_SSE4
           OR EMBREE_ISA_SUPPORTS_AVX
           OR EMBREE_ISA_SUPPORTS_AVX2
           OR EMBREE_ISA_SUPPORTS_AVX512KNL
           OR EMBREE_ISA_SUPPORTS_AVX512SKX))
      message(FATAL_ERROR
              "Your Embree build needs to support at least one ISA >= SSE4.1!")
  endif()
endmacro()
