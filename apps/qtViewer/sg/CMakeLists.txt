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

CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(ospray_sg STATIC
  # scene graph nodes
  SceneGraph.cpp
  Renderer.cpp
  common/TransferFunction.cpp
  geometry/Spheres.cpp
  geometry/TriangleMesh.cpp

  common/Common.cpp
  common/Node.cpp
  common/Integrator.cpp
  common/World.cpp
  common/Texture2D.cpp
  camera/PerspectiveCamera.cpp
  common/Material.cpp

  volume/Volume.cpp

  module/Module.cpp

  3rdParty/ply.cpp

  # scene graph importers
  importer/Importer.cpp
  importer/ImportSpheres.cpp
  importer/ImportOSP.cpp
  importer/ImportOBJ.cpp
  importer/ImportPLY.cpp
  importer/ImportRIVL.cpp
)

TARGET_LINK_LIBRARIES(ospray_sg
  ospray
  ospray_common
  ospray_xml
)
