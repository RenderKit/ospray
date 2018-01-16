## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

## VTK support ##

OPTION(OSPRAY_SG_VTK "Enable VTK loaders for ospray_sg" OFF)

IF(OSPRAY_SG_VTK)
  FIND_PACKAGE(VTK COMPONENTS REQUIRED
    vtkIOLegacy
    vtkIOXML
    vtkIOXMLParser
    vtkIOExport
    vtksys
  )

  INCLUDE(${VTK_USE_FILE})

  SET(SG_VTK_SRCS importer/importTetVolume.cpp)

  ADD_DEFINITIONS(-DOSPRAY_APPS_SG_VTK=1)
ENDIF()

## Chombo AMR loader support ##

OPTION(OSPRAY_SG_CHOMBO "AMR Chombo Reader (requires HDF5)" OFF)

IF (OSPRAY_SG_CHOMBO)
  FIND_PACKAGE(HDF5 REQUIRED)
  IF (HDF5_FOUND)
    INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIRS})
  ENDIF()
  SET(CHOMBO_SRCS importer/importAMRChombo.cpp)
  ADD_DEFINITIONS(-DOSPRAY_APPS_SG_CHOMBO=1)
ENDIF()

## OpenImageIO support ##

OPTION(OSPRAY_SG_OPENIMAGEIO "Enable OpenImageIO support" OFF)

IF(OSPRAY_SG_OPENIMAGEIO)
  FIND_PACKAGE(OpenImageIO 1.6.5)
  IF (OPENIMAGEIO_FOUND)
    ADD_DEFINITIONS(-DUSE_OPENIMAGEIO)
    INCLUDE_DIRECTORIES(${OPENIMAGEIO_INCLUDE_DIRS})
  ELSE()
    MESSAGE(STATUS "OpenImageIO not found. Using stb_image instead.")
  ENDIF()
ENDIF()

OSPRAY_CREATE_LIBRARY(ospray_sg SHARED
  # scene graph nodes
  SceneGraph.cpp
  Renderer.cpp

  geometry/Geometry.cpp
  geometry/Spheres.cpp
  geometry/Cylinders.cpp
  geometry/StreamLines.cpp
  geometry/TriangleMesh.cpp

  camera/Camera.h
  camera/PerspectiveCamera.cpp
  camera/PanoramicCamera.cpp
  camera/OrthographicCamera.cpp

  common/Common.cpp
  common/Data.cpp
  common/FrameBuffer.cpp
  common/Light.cpp
  common/Material.cpp
  common/Node.cpp
  common/Renderable.cpp
  common/RenderContext.h
  common/Serialization.cpp
  common/TimeStamp.cpp
  common/Texture2D.cpp
  common/Transform.cpp
  common/World.cpp
  common/Animator.cpp
  common/Animator.h
  common/AnimationController.h
  common/AnimationController.cpp
  common/Selector.h
  common/Selector.cpp

  transferFunction/TransferFunction.cpp
  volume/Volume.cpp
  volume/TetVolume.cpp
  volume/AMRVolume.cpp

  module/Module.cpp

  3rdParty/ply.cpp

  # scene graph importers
  importer/Importer.cpp
  importer/importPoints.cpp
  importer/importOSP.cpp
  importer/importOSPModel.cpp
  importer/importOSPSG.cpp
  importer/importOSX.cpp
  importer/importOBJ.cpp
  importer/importPLY.cpp
  importer/importRIVL.cpp
  importer/importXYZ.cpp

  importer/detail_xyz/Model.cpp

  ${SG_VTK_SRCS}
  ${CHOMBO_SRCS}

LINK
  ospray
  ospray_common
  ospray_xml
  ${HDF5_LIBRARIES}
  ${VTK_LIBRARIES}
  ${OPENIMAGEIO_LIBRARIES}
)
