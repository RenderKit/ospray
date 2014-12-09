## ======================================================================== ##
## Copyright 2009-2014 Intel Corporation                                    ##
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

SET(OSPRAY_MODULE_PATHTRACER OFF CACHE BOOL "Build 'pathtracer' (Embree Path Tracer) module")

IF (OSPRAY_MODULE_PATHTRACER)
  
  CONFIGURE_OSPRAY()
  CONFIGURE_MPI()
  
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)
  
  OSPRAY_ADD_LIBRARY(ospray_module_pathtracer${OSPRAY_LIB_SUFFIX} SHARED
    PathTracer.ispc
    PathTracer.cpp

    lights/Light.ispc
    lights/AmbientLight.ispc
    lights/PointLight.ispc
    lights/SpotLight.ispc
    
    materials/Material.ispc
    materials/OBJ.ispc
    materials/OBJ.cpp
    materials/Velvet.ispc
    materials/Velvet.cpp
    materials/Metal.ispc
    materials/Metal.cpp
    materials/ThinDielectric.ispc
    materials/ThinDielectric.cpp
    materials/Dielectric.ispc
    materials/Dielectric.cpp
    materials/MetallicPaint.ispc
    materials/MetallicPaint.cpp
    materials/Plastic.ispc
    materials/Plastic.cpp
    materials/Matte.ispc
    materials/Matte.cpp
    )
  TARGET_LINK_LIBRARIES(
    ospray_module_pathtracer${OSPRAY_LIB_SUFFIX} 
    ospray${OSPRAY_LIB_SUFFIX}
    )
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_module_pathtracer${OSPRAY_LIB_SUFFIX}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_module_pathtracer${OSPRAY_LIB_SUFFIX}
    DESTINATION lib)
ENDIF()
