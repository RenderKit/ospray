OSPRAY_CREATE_LIBRARY(ospray_module_sg_bilinear_patches
  moduleInit.cpp

  # this depends on ospray core:
  LINK
  ospray_sg
)
 

