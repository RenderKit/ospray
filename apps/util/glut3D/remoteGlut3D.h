/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "glut3D.h"

namespace ospray {
  namespace glut3D {
    struct RemoteGlut3D {
      static RemoteGlut3D *instance;

      RemoteGlut3D();

      void create() {};
      /*! draw local pixels remotely, in RBGA_I8 format */
      void drawPixels(const vec2i &size, const uint32 *pixels);

      // create a window (on the remote side) with given parameters
      void createWindow(const char *title, const vec2i &size, bool fullScreen);

      /*! the glut main loop */
      void mainLoop();
      void postRedisplay();
      /*! socket we're communicating over */
      int sockID;

      void sockRecv(void *ptr, int size);
      void sockSend(const void *ptr, int size);
    };
  }
}


