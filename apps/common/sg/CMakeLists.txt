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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

## VTK support ##

option(OSPRAY_SG_VTK "Enable VTK loaders for ospray_sg" OFF)

if(OSPRAY_SG_VTK)
  option(OSPRAY_SG_VTK_XDMF "Enable VTK XDMF loader for ospray_sg" OFF)
  if (OSPRAY_SG_VTK_XDMF)
    set(XDMF_COMPONENT vtkIOXdmf2)
    add_definitions(-DOSPRAY_APPS_SG_VTK_XDMF=1)
  endif()

  find_package(VTK COMPONENTS REQUIRED
    vtkIOLegacy
    vtkIOXML
    vtkIOXMLParser
    vtkIOExport
    vtksys
    ${XDMF_COMPONENT}
  )

  include(${VTK_USE_FILE})

  set(SG_VTK_SRCS
    importer/importUnstructuredVolume.cpp
    importer/importVTKPolyData.cpp
    importer/importVTI.cpp
    generator/generateVTKWaveletVolume.cpp
  )

  add_definitions(-DOSPRAY_APPS_SG_VTK=1)
endif()

## Chombo AMR loader support ##

option(OSPRAY_SG_CHOMBO "AMR Chombo Reader (requires HDF5)" OFF)

if(OSPRAY_SG_CHOMBO)
  find_package(HDF5 REQUIRED)
  if(HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIRS})
  endif()
  set(CHOMBO_SRCS importer/importAMRChombo.cpp)
  add_definitions(-DOSPRAY_APPS_SG_CHOMBO=1)
endif()

## OpenImageIO support ##

option(OSPRAY_SG_OPENIMAGEIO "Enable OpenImageIO support" OFF)

if(OSPRAY_SG_OPENIMAGEIO)
  find_package(OpenImageIO 1.6.5)
  if (OPENIMAGEIO_FOUND)
    add_definitions(-DUSE_OPENIMAGEIO)
    include_directories(${OPENIMAGEIO_INCLUDE_DIRS})
  else()
    message(STATUS "OpenImageIO not found. Using stb_image instead.")
  endif()
endif()

ospray_create_library(ospray_sg SHARED
  # scene graph nodes
  SceneGraph.cpp
  Renderer.cpp

  geometry/Curves.cpp
  geometry/Cylinders.cpp
  geometry/Geometry.cpp
  geometry/QuadMesh.cpp
  geometry/Slices.cpp
  geometry/Spheres.cpp
  geometry/StreamLines.cpp
  geometry/TriangleMesh.cpp

  camera/Camera.h
  camera/PerspectiveCamera.cpp
  camera/PanoramicCamera.cpp
  camera/OrthographicCamera.cpp

  common/Common.cpp
  common/Data.cpp
  common/FrameBuffer.cpp
  common/ToneMapper.cpp
  common/Light.cpp
  common/Material.cpp
  common/Node.cpp
  common/NodeParameter.cpp
  common/Renderable.cpp
  common/RenderContext.h
  common/Serialization.cpp
  common/Transform.cpp
  common/Model.cpp
  common/Instance.cpp
  common/Animator.cpp
  common/Animator.h
  common/AnimationController.h
  common/AnimationController.cpp
  common/Selector.h
  common/Selector.cpp

  transferFunction/TransferFunction.cpp
  volume/Volume.cpp
  volume/UnstructuredVolume.cpp
  volume/AMRVolume.cpp

  module/Module.cpp

  3rdParty/ply.cpp

  # scene graph generators
  generator/Generator.cpp
  generator/generateBasicVolume.cpp
  generator/generateCube.cpp
  generator/generateCurves.cpp
  generator/generateCylinders.cpp
  generator/generateGridOfSpheres.cpp
  generator/generateRandomQuads.cpp
  generator/generateRandomSpheres.cpp
  generator/generateUnstructuredVolume.cpp

  # scene graph importers
  importer/Importer.cpp
  importer/importPoints.cpp
  importer/importOSP.cpp
  importer/importX3D.cpp
  importer/importOSPSG.cpp
  importer/importOSX.cpp
  importer/importOBJ.cpp
  importer/importPLY.cpp
  importer/importRIVL.cpp
  importer/importXYZ.cpp

  importer/detail_xyz/Model.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/TextureVolume.cpp

  ${SG_VTK_SRCS}
  ${CHOMBO_SRCS}

LINK
  ospray
  ospray_common
  ospray_tfn
  ospray_json
  ${HDF5_LIBRARIES}
  ${VTK_LIBRARIES}
  ${OPENIMAGEIO_LIBRARIES}
)

## Install headers in SDK ##

set(OSPRAY_SG_SDK_INSTALL_LOC
  ../sg #NOTE: this is "next" to the SDK/ directory
)

ospray_install_sdk_headers(
  Renderer.h
  SceneGraph.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}
)

ospray_install_sdk_headers(
  camera/Camera.h
  camera/OrthographicCamera.h
  camera/PanoramicCamera.h
  camera/PerspectiveCamera.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/camera
)

ospray_install_sdk_headers(
  common/AnimationController.h
  common/Animator.h
  common/Common.h
  common/Data.h
  common/FrameBuffer.h
  common/ToneMapper.h
  common/Instance.h
  common/Light.h
  common/Material.h
  common/Model.h
  common/Node.h
  common/NodeList.h
  common/NodeParameter.h
  common/Renderable.h
  common/RenderContext.h
  common/RuntimeError.h
  common/Selector.h
  common/Serialization.h
  common/Transform.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/common
)

ospray_install_sdk_headers(
  generator/Generator.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/generator
)

ospray_install_sdk_headers(
  geometry/Curves.h
  geometry/Cylinders.h
  geometry/Geometry.h
  geometry/QuadMesh.h
  geometry/Spheres.h
  geometry/StreamLines.h
  geometry/TriangleMesh.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/geometry
)

ospray_install_sdk_headers(
  importer/Importer.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/importer
)

ospray_install_sdk_headers(
  module/Module.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/module
)

ospray_install_sdk_headers(
  texture/Texture.h
  texture/Texture2D.h
  texture/TextureVolume.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/texture
)

ospray_install_sdk_headers(
  transferFunction/TransferFunction.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/transferFunction
)

ospray_install_sdk_headers(
  visitor/GatherNodesByName.h
  visitor/GatherNodesByPosition.h
  visitor/MarkAllAsModified.h
  visitor/PrintNodes.h
  visitor/VerifyNodes.h
  visitor/Visitor.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/visitor
)

ospray_install_sdk_headers(
  volume/AMRVolume.h
  volume/UnstructuredVolume.h
  volume/Volume.h

  DESTINATION ${OSPRAY_SG_SDK_INSTALL_LOC}/volume
)
