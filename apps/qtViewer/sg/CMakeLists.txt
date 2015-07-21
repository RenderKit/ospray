CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${OSPRAY_SOURCE_DIR})

ADD_LIBRARY(ospray_sg${OSPRAY_LIB_SUFFIX} SHARED
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

  # scene graph importers
  importer/Importer.cpp
  importer/ImportSpheres.cpp
  importer/ImportOSP.cpp
  importer/ImportOBJ.cpp
  importer/ImportRIVL.cpp
  #    ImportX3D.cpp
)

TARGET_LINK_LIBRARIES(ospray_sg${OSPRAY_LIB_SUFFIX} 
  ospray_xml${OSPRAY_LIB_SUFFIX}
  ospray${OSPRAY_LIB_SUFFIX}
)



