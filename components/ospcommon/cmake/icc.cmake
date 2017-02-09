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

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -no-ansi-alias -std=c++11 -DNOMINMAX")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG -O3")
# on Windows use "-fp:fast" instead of "-fp-model fast"
#SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt -fma -no-inline-max-total-size -inline-factor=200 ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

IF (APPLE)
  SET (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} -dynamiclib)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7") # we only use MacOSX 10.7 features
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++") # link against C++11 stdlib
ENDIF()

# enable -static-intel and avoid to export ICC specific symbols from OSPRay
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs=ALL")
