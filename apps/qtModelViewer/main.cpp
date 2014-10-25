/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray
#include "ospray/ospray.h"
// qt
#include <iostream>
#include <QtGui>
#include <ctype.h>
// viewer
#include "ModelViewer.h"

namespace ospray {
  namespace viewer {
    void main(int argc, const char *argv[]) 
    {
      // init ospray
      ospInit(&argc,argv);
      // init qt
      QApplication *app = new QApplication(argc, (char **)argv);
      
      // Ref<sg::World> world;

      // create new modelviewer
      ModelViewer *modelViewer = new ModelViewer;

      for (int argID=1;argID<argc;argID++) {
        const std::string arg = argv[argID];
        if (arg[0] == '-') {
        } else {
          // std::cout << "#osp:qtv: reading RIVL file " << arg << std::endl;
          // world = sg::importRIVL(arg);
        }
      }

      modelViewer->show();
      // let qt run...
      app->exec();

      // done, closing down.
      delete modelViewer;
      delete app;
    }
  }
}

int main(int argc, const char *argv[]) 
{
  try {
    ospray::viewer::main(argc,argv);
    return 0;
  } catch (std::runtime_error e) {
    std::cerr << "#osp:mv: fatal error: " << e.what() << std::endl;
    return 1;
  }
}

