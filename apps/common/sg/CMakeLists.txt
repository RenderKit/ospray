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

## VTK support ##

option(OSPRAY_SG_VTK "Enable VTK loaders for ospray_sg" OFF)

if(OSPRAY_SG_VTK)
  option(OSPRAY_SG_VTK_XDMF "Enable VTK XDMF loader for ospray_sg" OFF)
  if (OSPRAY_SG_VTK_XDMF)
    set(XDMF_COMPONENT vtkIOXdmf2)
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
endif()

## Chombo AMR loader support ##

option(OSPRAY_SG_CHOMBO "AMR Chombo Reader (requires HDF5)" OFF)

if(OSPRAY_SG_CHOMBO)
  find_package(HDF5 REQUIRED)
  set(CHOMBO_SRCS importer/importAMRChombo.cpp)
endif()

## OpenImageIO support ##

option(OSPRAY_SG_OPENIMAGEIO "Enable OpenImageIO support" OFF)

if(OSPRAY_SG_OPENIMAGEIO)
  find_package(OpenImageIO 1.6.5)
  if (NOT OPENIMAGEIO_FOUND)
    message(STATUS "OpenImageIO not found. Using stb_image instead.")
  endif()
endif()

## OpenImageDenoise support ##

option(OSPRAY_APPS_ENABLE_DENOISER "Use image denoiser in example viewer")
if (OSPRAY_APPS_ENABLE_DENOISER)
  find_package(OpenImageDenoise 0.8 REQUIRED)
  set(OPENIMAGEDENOISE OpenImageDenoise)
endif()

## Build Library ##

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
)

target_link_libraries(ospray_sg
PUBLIC
  ospray
  ospray_common
  ospray_tfn
  ospray_json
  ${HDF5_LIBRARIES}
  ${VTK_LIBRARIES}
  ${OPENIMAGEIO_LIBRARIES}
  ${OPENIMAGEDENOISE}
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/sg>
)

## Setup additional compile config for optional dependencies ##

if(OSPRAY_SG_CHOMBO)
  target_include_directories(ospray_sg PRIVATE ${HDF5_INCLUDE_DIRS})
  target_compile_definitions(ospray_sg PUBLIC -DOSPRAY_APPS_SG_CHOMBO=1)
endif()

if(OSPRAY_SG_OPENIMAGEIO AND OPENIMAGEIO_FOUND)
  target_include_directories(ospray_sg PRIVATE ${OPENIMAGEIO_INCLUDE_DIRS})
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENIMAGEIO)
endif()

if(OSPRAY_SG_VTK)
  if (OSPRAY_SG_VTK_XDMF)
    target_compile_definitions(ospray_sg PRIVATE -DOSPRAY_APPS_SG_VTK_XDMF=1)
  endif()

  target_compile_definitions(ospray_sg PUBLIC -DOSPRAY_APPS_SG_VTK=1)
endif()

if (OSPRAY_APPS_ENABLE_DENOISER)
  target_compile_definitions(ospray_sg PUBLIC -DOSPRAY_APPS_ENABLE_DENOISER=1)

  ## redistribute OpenImageDenoise
  if (OSPRAY_INSTALL_DEPENDENCIES)
    get_target_property(OIDN_LIB OpenImageDenoise IMPORTED_LOCATION_RELEASE)
    if (WIN32)
      install(PROGRAMS ${OIDN_LIB}
              DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
    else()
      install(PROGRAMS ${OIDN_LIB}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)

      get_filename_component(OIDN_LIBNAME ${OIDN_LIB} NAME)
      ospray_install_namelink(OpenImageDenoise ${OIDN_LIBNAME})
    endif()
  endif()
endif()

## Install headers in SDK ##

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
  PATTERN *3rdParty* EXCLUDE
  PATTERN importer/detail_xyz EXCLUDE
)
