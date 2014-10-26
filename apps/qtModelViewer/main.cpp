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
// scene graph
#include "sg/SceneGraph.h"

namespace ospray {
  namespace viewer {
    using std::cout;
    using std::endl;

    /*! @{ state to be set via commandline params */
    
    /*! file we're saving the output to. If empty, we'll open a
      interactive viewer; otherwise we'll render to this file and
      exit. */
    std::string outFileName = "";

    /*! size of frame (when rendering offline) resp render widget
        window (when creating viewer) */
    vec2i frameResolution(1024,768);

    /*! camera as specified on the command line */
    Ref<sg::Camera> cameraFromCommandLine = NULL;
    /*! @} */

    void main(int argc, const char *argv[]) 
    {
      // init ospray
      ospInit(&argc,argv);
      // init qt
      QApplication *app = new QApplication(argc, (char **)argv);
      
      Ref<sg::World> world;
      Ref<sg::Renderer> renderer = new sg::Renderer;

      for (int argID=1;argID<argc;argID++) {
        const std::string arg = argv[argID];
        if (arg[0] == '-') {
          if (arg == "-o" || arg == "--render-to-file") {
            outFileName = argv[++argID];
          } else if (arg == "--size") {
            frameResolution.x = atoi(argv[++argID]);
            frameResolution.y = atoi(argv[++argID]);
          } else if (arg == "--test-sphere") {
            world = sg::createTestSphere();
          } else {
            throw std::runtime_error("#ospQTV: unknown cmdline param '"+arg+"'");
          }
        } else {
              
          // std::cout << "#osp:qtv: reading RIVL file " << arg << std::endl;
          //world = sg::importRIVL(arg);
        }
      }

      // set the current world ...
      renderer->setWorld(world);
      
      // activate the last camera defined in the scene graph (if set)
      if (cameraFromCommandLine) {
        renderer->setCamera(cameraFromCommandLine);
      } else {
        renderer->setCamera(renderer->getLastDefinedCamera());
      }
      if (!renderer->camera)
        renderer->setDefaultCamera();

      if (outFileName == "") {
        // create new modelviewer
        cout << "#ospQTV: setting up to open QT viewer window" << endl;
        ModelViewer *modelViewer = new ModelViewer(renderer);
        modelViewer->show();
        // let qt run...
        app->exec();
        
        // done, closing down.
        delete modelViewer;
        delete app;
      } else {
        if (!renderer->frameBuffer)
          renderer->frameBuffer = new sg::FrameBuffer(frameResolution);

        // output file specified - render to file
        cout << "#ospQTV: setting up in render-to-file mode" << endl;
        renderer->renderFrame();
        unsigned char *fbMem = renderer->frameBuffer->map();
        QImage image(fbMem,
                     renderer->frameBuffer->getSize().x,
                     renderer->frameBuffer->getSize().y,
                     QImage::Format_ARGB32);
        image.save(outFileName.c_str());
        renderer->frameBuffer->unmap(fbMem);
      }
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
  
