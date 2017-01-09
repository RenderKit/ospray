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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

OSPRAY_CREATE_LIBRARY(ospray_sg SHARED
  # scene graph nodes
  SceneGraph.cpp
  Renderer.cpp

  geometry/Spheres.cpp
  geometry/TriangleMesh.cpp

  camera/PerspectiveCamera.cpp

  common/Serialization.cpp
  common/Common.cpp
  common/Node.cpp
  common/Integrator.cpp
  common/World.cpp
  common/Texture2D.cpp
  common/Material.cpp
  common/Transform.cpp
  common/FrameBuffer.cpp
  
  transferFunction/TransferFunction.cpp
  volume/Volume.cpp

  module/Module.cpp

  3rdParty/ply.cpp

  # scene graph importers
  importer/Importer.cpp
  importer/ImportOSG.cpp
  importer/ImportOBJ.cpp
  importer/ImportPLY.cpp
  importer/ImportRIVL.cpp
LINK
  ospray
  ospray_common
  ospray_xml
)
