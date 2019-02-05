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

# lits of components that have already been built. we use this for
# dependencies tracking: any part of ospray that 'depends' on a given
# component can simply call 'OSPRAY_BUILD_COMPONENT(...)' without
# having to worry about whether this component is already built - if
# it isn't, then it is going to be built by calling this macro; while
# if it already is built, we don't have to worry about building it
# twice.

set(OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS "" CACHE INTERNAL "" FORCE)
set(OSPRAY_COMPONENTS_ROOT "${PROJECT_SOURCE_DIR}/components" CACHE INTERNAL "" FORCE)

# the macro any part of ospray can use to request ospray to
# include/build a specific component
macro(ospray_build_component comp)
  set(CURRENT_COMPONENT_DIR ${COMPONENTS_DIR}/${comp})
  if(NOT EXISTS ${CURRENT_COMPONENT_DIR})
    message(FATAL_ERROR "Could not find component '${comp}'!")
  endif()

  if (";${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS};" MATCHES ";${comp};")
    # component already built; nothing to do!
  else()
    set(INCLUDED_AS_AN_OSPRAY_COMPONENT ON)
    set(OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS
        ${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS} ${comp}
        CACHE INTERNAL "" FORCE)

    # make sure that components explicitly set their install COMPONENT
    # otherwise they would inherit the default COMPONENT of the
    # requesting target, leading to inconsistent behaviour
    set(DEFAULT_COMPONENT ${OSPRAY_DEFAULT_COMPONENT})
    unset(OSPRAY_DEFAULT_COMPONENT)

    add_subdirectory(${CURRENT_COMPONENT_DIR}
      ${CMAKE_BINARY_DIR}/built_components/${comp} ${ARGN})

    set(OSPRAY_DEFAULT_COMPONENT ${DEFAULT_COMPONENT})

  endif()

    if(EXISTS ${CURRENT_COMPONENT_DIR}/include.cmake)
      include(${CURRENT_COMPONENT_DIR}/include.cmake)
    endif()

endmacro()
