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

set(OSPRAY_CXX_FLAGS
    "-fPIC -fno-strict-aliasing -Wno-narrowing -Wno-unknown-pragmas")

if(OSPRAY_STRICT_BUILD)
  # Should try to fix and remove...
  set(OSPRAY_CXX_FLAGS "-Wno-aligned-new ${OSPRAY_CXX_FLAGS}")

  # Options selected for GCC 7.1+
  set(OSPRAY_CXX_FLAGS "-Wall ${OSPRAY_CXX_FLAGS}")
endif()

if(OSPRAY_WARN_AS_ERRORS)
  set(OSPRAY_CXX_FLAGS "-Werror ${OSPRAY_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g     -Wstrict-aliasing=1")
set(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=1 -ffast-math -fno-finite-math-only ")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=1 -ffast-math -fno-finite-math-only ")

if (APPLE)
  set(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.9 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.9 features
endif()
