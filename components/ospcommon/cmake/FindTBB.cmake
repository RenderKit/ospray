## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

SET(TBB_VERSION_REQUIRED "3.0")

IF (NOT TBB_ROOT)
  SET(TBB_ROOT $ENV{TBB_ROOT})
ENDIF()
IF (NOT TBB_ROOT)
  SET(TBB_ROOT $ENV{TBBROOT})
ENDIF()

# detect changed TBB_ROOT
IF (NOT TBB_ROOT STREQUAL TBB_ROOT_LAST)
  UNSET(TBB_INCLUDE_DIR CACHE)
  UNSET(TBB_LIBRARY CACHE)
  UNSET(TBB_LIBRARY_DEBUG CACHE)
  UNSET(TBB_LIBRARY_MALLOC CACHE)
  UNSET(TBB_LIBRARY_MALLOC_DEBUG CACHE)
ENDIF()

IF (WIN32)
  # workaround for parentheses in variable name / CMP0053
  SET(PROGRAMFILESx86 "PROGRAMFILES(x86)")
  SET(PROGRAMFILES32 "$ENV{${PROGRAMFILESx86}}")
  IF (NOT PROGRAMFILES32)
    SET(PROGRAMFILES32 "$ENV{PROGRAMFILES}")
  ENDIF()
  IF (NOT PROGRAMFILES32)
    SET(PROGRAMFILES32 "C:/Program Files (x86)")
  ENDIF()
  FIND_PATH(TBB_ROOT include/tbb/task_scheduler_init.h
    DOC "Root of TBB installation"
    HINTS ${TBB_ROOT}
    PATHS
      ${PROJECT_SOURCE_DIR}/tbb
      ${PROJECT_SOURCE_DIR}/../tbb
      "${PROGRAMFILES32}/IntelSWTools/compilers_and_libraries/windows/tbb"
      "${PROGRAMFILES32}/Intel/Composer XE/tbb"
      "${PROGRAMFILES32}/Intel/compilers_and_libraries/windows/tbb"
  )

  IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(TBB_ARCH intel64)
  ELSE()
    SET(TBB_ARCH ia32)
  ENDIF()

  IF (MSVC10)
    SET(TBB_VCVER vc10)
  ELSEIF (MSVC11)
    SET(TBB_VCVER vc11)
  ELSEIF (MSVC12)
    SET(TBB_VCVER vc12)
  ELSE()
    SET(TBB_VCVER vc14)
  ENDIF()

  SET(TBB_LIBDIR ${TBB_ROOT}/lib)

  FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY tbb
    PATHS
    ${TBB_LIBDIR}/${TBB_ARCH}/${TBB_VCVER}
    ${TBB_LIBDIR}
    NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_DEBUG tbb_debug
    PATHS
    ${TBB_LIBDIR}/${TBB_ARCH}/${TBB_VCVER}
    ${TBB_LIBDIR}
    NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc
    PATHS
    ${TBB_LIBDIR}/${TBB_ARCH}/${TBB_VCVER}
    ${TBB_LIBDIR}
    NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC_DEBUG tbbmalloc_debug
    PATHS
    ${TBB_LIBDIR}/${TBB_ARCH}/${TBB_VCVER}
    ${TBB_LIBDIR}
    NO_DEFAULT_PATH)

ELSE ()

  FIND_PATH(TBB_ROOT include/tbb/task_scheduler_init.h
    DOC "Root of TBB installation"
    HINTS ${TBB_ROOT}
    PATHS
      ${PROJECT_SOURCE_DIR}/tbb
      /opt/intel/composerxe/tbb
      /opt/intel/compilers_and_libraries/tbb
  )

  IF (APPLE)
    FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include NO_DEFAULT_PATH)
    FIND_LIBRARY(TBB_LIBRARY tbb PATHS ${TBB_ROOT}/lib NO_DEFAULT_PATH)
    FIND_LIBRARY(TBB_LIBRARY_DEBUG tbb_debug PATHS ${TBB_ROOT}/lib NO_DEFAULT_PATH)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc PATHS ${TBB_ROOT}/lib NO_DEFAULT_PATH)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC_DEBUG tbbmalloc_debug PATHS ${TBB_ROOT}/lib NO_DEFAULT_PATH)
  ELSE()
    FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include NO_DEFAULT_PATH)
    SET(TBB_HINTS HINTS ${TBB_ROOT}/lib/intel64/gcc4.4 ${TBB_ROOT}/lib ${TBB_ROOT}/lib64 PATHS /usr/libx86_64-linux-gnu/)
    FIND_LIBRARY(TBB_LIBRARY libtbb.so.2 ${TBB_HINTS})
    FIND_LIBRARY(TBB_LIBRARY_DEBUG libtbb_debug.so.2 ${TBB_HINTS})
    FIND_LIBRARY(TBB_LIBRARY_MALLOC libtbbmalloc.so.2 ${TBB_HINTS})
    FIND_LIBRARY(TBB_LIBRARY_MALLOC_DEBUG libtbbmalloc_debug.so.2 ${TBB_HINTS})
  ENDIF()
ENDIF()

SET(TBB_ROOT_LAST ${TBB_ROOT} CACHE INTERNAL "Last value of TBB_ROOT to detect changes")

SET(TBB_ERROR_MESSAGE
  "Threading Building Blocks (TBB) with minimum version ${TBB_VERSION_REQUIRED} not found.
OSPRay uses TBB as default tasking system. Please make sure you have the TBB headers installed as well (the package is typically named 'libtbb-dev' or 'tbb-devel') and/or hint the location of TBB in TBB_ROOT.
Alternatively, you can try to use OpenMP as tasking system by setting OSPRAY_TASKING_SYSTEM=OpenMP")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TBB
  ${TBB_ERROR_MESSAGE}
  TBB_INCLUDE_DIR TBB_LIBRARY TBB_LIBRARY_MALLOC
)

# check version
IF (TBB_INCLUDE_DIR)
  FILE(READ ${TBB_INCLUDE_DIR}/tbb/tbb_stddef.h TBB_STDDEF_H)

  STRING(REGEX MATCH "#define TBB_VERSION_MAJOR ([0-9]+)" DUMMY "${TBB_STDDEF_H}")
  SET(TBB_VERSION_MAJOR ${CMAKE_MATCH_1})

  STRING(REGEX MATCH "#define TBB_VERSION_MINOR ([0-9]+)" DUMMY "${TBB_STDDEF_H}")
  SET(TBB_VERSION "${TBB_VERSION_MAJOR}.${CMAKE_MATCH_1}")

  IF (TBB_VERSION VERSION_LESS TBB_VERSION_REQUIRED)
    MESSAGE(FATAL_ERROR ${TBB_ERROR_MESSAGE})
  ENDIF()

  SET(TBB_VERSION ${TBB_VERSION} CACHE STRING "TBB Version")
  MARK_AS_ADVANCED(TBB_VERSION)
ENDIF()

IF (TBB_FOUND)
  SET(TBB_INCLUDE_DIRS ${TBB_INCLUDE_DIR})
  # NOTE(jda) - TBB found in CentOS 6/7 package manager does not have debug
  #             versions of the library...silently fall-back to using only the
  #             libraries which we actually found.
  IF (NOT TBB_LIBRARY_DEBUG)
    SET(TBB_LIBRARIES ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC})
  ELSE ()
    SET(TBB_LIBRARIES
        optimized ${TBB_LIBRARY} optimized ${TBB_LIBRARY_MALLOC}
        debug ${TBB_LIBRARY_DEBUG} debug ${TBB_LIBRARY_MALLOC_DEBUG}
    )
  ENDIF()
ENDIF()

MARK_AS_ADVANCED(TBB_INCLUDE_DIR)
MARK_AS_ADVANCED(TBB_LIBRARY)
MARK_AS_ADVANCED(TBB_LIBRARY_DEBUG)
MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC)
MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC_DEBUG)
