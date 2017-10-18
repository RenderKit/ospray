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

SET(OSPRAY_CXX_FLAGS "-std=c++11 -fPIC -fno-strict-aliasing -no-ansi-alias -DNOMINMAX -Wno-unknown-pragmas")

IF(OSPRAY_STRICT_BUILD)
  SET(OSPRAY_CXX_FLAGS "-Wall ${OSPRAY_CXX_FLAGS}")
ENDIF()

SET(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG -O3")
# on Windows use "-fp:fast" instead of "-fp-model fast"
#SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt -fma -no-inline-max-total-size -inline-factor=200 ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g ${CMAKE_CXX_FLAGS_RELEASE}")
SET(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

IF (APPLE)
  SET (CMAKE_SHARED_LINKER_FLAGS "-dynamiclib ${CMAKE_SHARED_LINKER_FLAGS_INIT}")
  SET(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.8 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.8 features
  SET(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}") # link against C++11 stdlib
ENDIF()

# enable -static-intel and avoid to export ICC specific symbols from OSPRay
SET(CMAKE_CXX_FLAGS "-static-intel ${CMAKE_CXX_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--exclude-libs=ALL ${CMAKE_SHARED_LINKER_FLAGS}")
