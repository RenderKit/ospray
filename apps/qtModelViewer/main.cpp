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
// embree
#include "ospray/embree/common/sys/filename.h"
// scene graph
#include "sg/SceneGraph.h"

namespace ospray {
  namespace viewer {
    using std::cout;
    using std::endl;

    static const std::string DEFAULT_INTEGRATOR_NAME = "ao4";
    // static const std::string DEFAULT_INTEGRATOR_NAME = "eyeLight_geomID";
    

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

    /*! renderer as specified on the command line */
    std::string integratorFromCommandLine = "";

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
          } else if (arg == "--test-axes") {
            // world = sg::createTestCoordinateAxes();
          } else if (arg == "--renderer") {
            integratorFromCommandLine = argv[++argID];
          } else {
            throw std::runtime_error("#ospQTV: unknown cmdline param '"+arg+"'");
          }
        } else {
          embree::FileName fn = arg;
          if (fn.ext() == "osp") {
            world = sg::loadOSP(fn.str());
          } else 
            throw std::runtime_error("unsupported file format in '"+fn.str()+"'");
          // std::cout << "#osp:qtv: reading RIVL file " << arg << std::endl;
          //world = sg::importRIVL(arg);
        }
      }

      // set the current world ...
      renderer->setWorld(world);
      
      // -------------------------------------------------------
      // initialize renderer's integrator
      // -------------------------------------------------------
      {
        // first, check if one is specified in the scene file.
        Ref<sg::Integrator> integrator = renderer->getLastDefinedIntegrator();
        if (!integrator) {
          std::string integratorName = integratorFromCommandLine;
          if (integratorName == "")
            integratorName = DEFAULT_INTEGRATOR_NAME;
          integrator = new sg::Integrator(integratorName);
        }
        renderer->setIntegrator(integrator);
      }

      // -------------------------------------------------------
      // initialize renderer's camera
      // -------------------------------------------------------
      {
        // activate the last camera defined in the scene graph (if set)
        if (cameraFromCommandLine) {
          renderer->setCamera(cameraFromCommandLine);
        } else {
          renderer->setCamera(renderer->getLastDefinedCamera());
        }
        if (!renderer->camera) {
          renderer->setDefaultCamera();
        }
      }


      // -------------------------------------------------------
      // determine output method: offline to file, or interactive viewer
      // -------------------------------------------------------
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
        cout << "#ospQTV: setting up in render-to-file mode" << endl;
        if (!renderer->frameBuffer) {
          cout << "#ospQTV: creating default framebuffer (" 
               << frameResolution.x << "x" << frameResolution.y << ")" << endl;
          renderer->frameBuffer = new sg::FrameBuffer(frameResolution);
        }

        // output file specified - render to file
        cout << "#ospQTV: rendering frame" << endl;
        renderer->renderFrame();

        unsigned char *fbMem = renderer->frameBuffer->map();
        cout << "#ospQTV: saving image" << endl;
        QImage image(fbMem,
                     renderer->frameBuffer->getSize().x,
                     renderer->frameBuffer->getSize().y,
                     QImage::Format_ARGB32);
        image.save(outFileName.c_str());
        renderer->frameBuffer->unmap(fbMem);
        cout << "#ospQTV: rendered image saved to " << outFileName << endl;
        cout << "#ospQTV: done... exiting." << endl;
        exit(0);
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
  
