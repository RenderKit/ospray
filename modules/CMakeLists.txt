# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################


FILE(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
FOREACH(plugin ${plugins})
  IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
    ADD_SUBDIRECTORY(${plugin})
  ENDIF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
ENDFOREACH(plugin ${plugins})

