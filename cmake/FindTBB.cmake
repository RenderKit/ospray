## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

IF (NOT TBB_ROOT)
  SET(TBB_ROOT $ENV{TBB_ROOT})
ENDIF()
IF (NOT TBB_ROOT)
  SET(TBB_ROOT $ENV{TBBROOT})
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
  ELSE()
    SET(TBB_VCVER vc12)
  ENDIF()

  SET(TBB_LIBDIR ${TBB_ROOT}/lib/${TBB_ARCH}/${TBB_VCVER})
  SET(TBB_BINDIR ${TBB_ROOT}/bin/${TBB_ARCH}/${TBB_VCVER})

  FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY tbb PATHS ${TBB_LIBDIR} NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_DEBUG tbb_debug PATHS ${TBB_LIBDIR} NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC tbbmalloc PATHS ${TBB_LIBDIR} NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC_DEBUG tbbmalloc_debug PATHS ${TBB_LIBDIR} NO_DEFAULT_PATH)

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
    FIND_LIBRARY(TBB_LIBRARY libtbb.so.2 HINTS ${TBB_ROOT}/lib/intel64/gcc4.4)
    FIND_LIBRARY(TBB_LIBRARY_DEBUG libtbb_debug.so.2 HINTS ${TBB_ROOT}/lib/intel64/gcc4.4)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC libtbbmalloc.so.2 HINTS ${TBB_ROOT}/lib/intel64/gcc4.4)
    FIND_LIBRARY(TBB_LIBRARY_MALLOC_DEBUG libtbbmalloc_debug.so.2 HINTS ${TBB_ROOT}/lib/intel64/gcc4.4)
  ENDIF()

  FIND_PATH(TBB_INCLUDE_DIR_MIC tbb/task_scheduler_init.h PATHS ${TBB_ROOT}/include NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MIC libtbb.so.2 PATHS ${TBB_ROOT}/lib/mic NO_DEFAULT_PATH)
  FIND_LIBRARY(TBB_LIBRARY_MALLOC_MIC libtbbmalloc.so.2 PATHS ${TBB_ROOT}/lib/mic NO_DEFAULT_PATH)

  MARK_AS_ADVANCED(TBB_INCLUDE_DIR_MIC)
  MARK_AS_ADVANCED(TBB_LIBRARY_MIC)
  MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC_MIC)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TBB
  "Threading Building Blocks (TBB) not found.
OSPRay uses TBB as default tasking system. Please make sure you have the TBB headers installed as well (the package is typically named 'libtbb-dev' or 'tbb-devel') and/or hint the location of TBB in TBB_ROOT.
Alternatively, you can try to use OpenMP as tasking system by setting OSPRAY_TASKING_SYSTEM=OpenMP"
  TBB_INCLUDE_DIR TBB_LIBRARY TBB_LIBRARY_MALLOC
)

IF (TBB_FOUND)
  SET(TBB_INCLUDE_DIRS ${TBB_INCLUDE_DIR})
  SET(TBB_LIBRARIES
      optimized ${TBB_LIBRARY} optimized ${TBB_LIBRARY_MALLOC}
      debug ${TBB_LIBRARY_DEBUG} debug ${TBB_LIBRARY_MALLOC_DEBUG}
  )
ENDIF()

IF (TBB_INCLUDE_DIR AND TBB_LIBRARY_MIC AND TBB_LIBRARY_MALLOC_MIC)
  SET(TBB_FOUND_MIC TRUE)
  SET(TBB_INCLUDE_DIRS_MIC ${TBB_INCLUDE_DIR_MIC})
  SET(TBB_LIBRARIES_MIC ${TBB_LIBRARY_MIC} ${TBB_LIBRARY_MALLOC_MIC})
ENDIF()

MARK_AS_ADVANCED(TBB_INCLUDE_DIR)
MARK_AS_ADVANCED(TBB_LIBRARY)
MARK_AS_ADVANCED(TBB_LIBRARY_DEBUG)
MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC)
MARK_AS_ADVANCED(TBB_LIBRARY_MALLOC_DEBUG)
