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

set(OSPRAY_MPI_INCLUDES ${MPI_CXX_INCLUDE_PATH})
