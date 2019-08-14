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

set(OSPRAY_CXX_FLAGS "-fPIC -fno-strict-aliasing -Wno-narrowing")

if(OSPRAY_STRICT_BUILD)
  # OK to turn off.
  set(OSPRAY_CXX_FLAGS "-Wno-c++98-compat-pedantic ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-documentation ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-documentation-unknown-command ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-zero-as-null-pointer-constant ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-newline-eof ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-keyword-macro ${OSPRAY_CXX_FLAGS}") #useful for unit testing

  # Should try to fix and remove...
  set(OSPRAY_CXX_FLAGS "-Wno-unknown-warning-option ${OSPRAY_CXX_FLAGS}") #don't warn if pragmas are unknown
  set(OSPRAY_CXX_FLAGS "-Wno-conversion ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-reserved-id-macro ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-double-promotion ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-used-but-marked-unused ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-old-style-cast ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-missing-noreturn ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-missing-prototypes ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shift-sign-overflow ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-padded ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shadow-field-in-constructor ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-weak-template-vtables ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-weak-vtables ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-exit-time-destructors ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-global-constructors ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-unused-template ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-switch-enum ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-float-equal ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-cast-align ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-deprecated ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-mismatched-tags ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-disabled-macro-expansion ${OSPRAY_CXX_FLAGS}") #pesky 'stb_image.h'...
  set(OSPRAY_CXX_FLAGS "-Wno-over-aligned ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shadow ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-format-nonliteral ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-cast-qual ${OSPRAY_CXX_FLAGS}") #Embree v3.x issue

  # Options selected for Clang 5.0+
  set(OSPRAY_CXX_FLAGS "-Weverything ${OSPRAY_CXX_FLAGS}")
endif()

if(OSPRAY_WARN_AS_ERRORS)
  set(OSPRAY_CXX_FLAGS "-Werror ${OSPRAY_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g -O0 -Wstrict-aliasing=1")
set(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=1")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=1")

if (APPLE)
  set(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.9 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.9 features
endif()
