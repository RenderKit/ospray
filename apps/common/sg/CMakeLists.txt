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
  ${OPENIMAGEDENOISE}
)

## Install headers in SDK ##

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  FILES_MATCHING
  PATTERN *.h
)
