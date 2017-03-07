// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// viewer
#include "ModelViewer.h"
// ospray
#include "ospray/ospray.h"
// qt
#include <iostream>
#include <QtGui>
#include <ctype.h>
// scene graph
#include "sg/module/Module.h"
#include "sg/importer/Importer.h"
#include "sg/common/FrameBuffer.h"

#include <fstream>
#include <stdexcept>

namespace ospray {
  namespace viewer {
    using std::cout;
    using std::endl;

    /*! verbosity level */
    int verbosity = 0;
    
    static const std::string DEFAULT_INTEGRATOR_NAME = "scivis"; //ao2";
    // static const std::string DEFAULT_INTEGRATOR_NAME = "eyeLight_geomID";
    

    /*! @{ state to be set via commandline params */
    
    /*! file we're saving the output to. If empty, we'll open a
      interactive viewer; otherwise we'll render to this file and
      exit. */
    std::string outFileName = "";

    /*! size of frame (when rendering offline) resp render widget
      window (when creating viewer) */
    vec2i frameResolution(-1,-1);

    /*! camera as specified on the command line */
    std::shared_ptr<sg::PerspectiveCamera> cameraFromCommandLine;
    vec3f upFromCommandLine(0,0,0);

    /*! renderer as specified on the command line */
    std::string integratorFromCommandLine = "";

    /*! whether we will display the frames per second */
    bool showFPS = false;

    /*! number of samples per pixel */
    int spp = 1;

    /*! @} */

    void main(int argc, const char *argv[]) 
    {
      // init ospray
      ospInit(&argc,argv);
      auto device = ospGetCurrentDevice();
      ospDeviceSetErrorMsgFunc(device,
                               [](const char *msg) { std::cout << msg; });
      // init qt
      QApplication *app = new QApplication(argc, (char **)argv);
      
      auto renderer_h = sg::createNode("renderer", "Renderer");
      auto world_h    = renderer_h["world"];

      std::shared_ptr<sg::Renderer> renderer =
        std::dynamic_pointer_cast<sg::Renderer>(renderer_h.get());

      std::shared_ptr<sg::World> world =
        std::dynamic_pointer_cast<sg::World>(world_h.get());

      bool fullscreen = false;

      for (int argID=1;argID<argc;argID++) {
        const std::string arg = argv[argID];
        if (arg[0] == '-') {
          if (arg == "-o" || arg == "--render-to-file") {
            outFileName = argv[++argID];
          } else if (arg == "--size") {
            frameResolution.x = atoi(argv[++argID]);
            frameResolution.y = atoi(argv[++argID]);
          } else if (arg == "-spp" ||
                     arg == "--spp" ||
                     arg == "--samples-per-pixel") {
            spp = atoi(argv[++argID]);
          } else if (arg == "--data-distributed" || arg == "--data-parallel") {
            sg::Volume::useDataDistributedVolume = true;
          } else if (arg == "--1k" || arg == "-1k") {
            frameResolution.x = 1024;
            frameResolution.y = 1024;
          } else if (arg == "--show-fps" || arg == "-fps" || arg == "--fps") {
            showFPS = true;
          } else if (arg == "--module") {
            sg::loadModule(argv[++argID]);
          } else if (arg == "--renderer") {
            integratorFromCommandLine = argv[++argID];
          } else if (arg == "-v") {
            verbosity = 1;
          } else if (arg == "--view-file") {
            std::ifstream fin(argv[++argID]);
            if (!fin.is_open())
            {
              throw std::runtime_error("Failed to open \"" +
                                       std::string(argv[argID]) +
                                       "\" for reading");
            }

            if (!cameraFromCommandLine)
              cameraFromCommandLine = std::make_shared<sg::PerspectiveCamera>();

            auto x = 0.f;
            auto y = 0.f;
            auto z = 0.f;

            auto token = std::string("");
            while (fin >> token)
            {
              if (token == "-vp")
              {
                fin >> x >> y >> z;
                cameraFromCommandLine->setFrom(vec3f(x,y,z));
              }
              else if (token == "-vu")
              {
                fin >> x >> y >> z;
                cameraFromCommandLine->setUp(vec3f(x,y,z));
              }
              else if (token == "-vi")
              {
                fin >> x >> y >> z;
                upFromCommandLine = vec3f(x, y, z);
                cameraFromCommandLine->setAt(upFromCommandLine);
              }
              else if (token == "-fv")
              {
                fin >> x;
                cameraFromCommandLine->setFovy(x);
              }
              else
              {
                throw std::runtime_error("Unrecognized token:  \"" + token +
                                         '\"');
              }
            }
          } else if (arg == "-vi") {
            if (!cameraFromCommandLine)
              cameraFromCommandLine = std::make_shared<sg::PerspectiveCamera>();
            assert(argID+3<argc);
            float x = atof(argv[++argID]);
            float y = atof(argv[++argID]);
            float z = atof(argv[++argID]);
            cameraFromCommandLine->setAt(vec3f(x,y,z));
          } else if (arg == "-vp") {
            if (!cameraFromCommandLine)
              cameraFromCommandLine = std::make_shared<sg::PerspectiveCamera>();
            assert(argID+3<argc);
            float x = atof(argv[++argID]);
            float y = atof(argv[++argID]);
            float z = atof(argv[++argID]);
            cameraFromCommandLine->setFrom(vec3f(x,y,z));
          } else if (arg == "-vu") {
            assert(argID+3<argc);
            float x = atof(argv[++argID]);
            float y = atof(argv[++argID]);
            float z = atof(argv[++argID]);
            upFromCommandLine = vec3f(x,y,z);
            if (cameraFromCommandLine)
              cameraFromCommandLine->setUp(vec3f(x,y,z));
          } else if (arg == "--fullscreen" || arg == "-fs"){
            fullscreen = true;
          } else {
            throw std::runtime_error("#osp:qtv: unknown cmdline param '" +
                                     arg + "'");
          }
        } else {
          FileName fn = arg;
          if (fn.ext() == "osp" || fn.ext() == "osg" || fn.ext() == "pkd") {
            world = sg::loadOSP(fn.str());
            // } else if (fn.ext() == "atom") {
            //   world = sg::AlphaSpheres::importOspAtomFile(fn.str());
          } else if ((fn.ext() == "ply") || 
                     ((fn.ext() == "gz") && (fn.dropExt().ext() == "ply"))) {
            sg::importPLY(world,fn);
          } else if (fn.ext() == "obj") {
            sg::importOBJ(world,fn);
          } else if (fn.ext() == "xml") {
            std::cout << "#osp:qtv: reading RIVL file " << arg << std::endl;
            world = sg::importRIVL(arg);
          } else 
            sg::importFile(world,fn);
        }
      }

      if (!world) {
        std::cout << "#osp:qtv: no world defined. exiting." << std::endl;
        exit(1);
      }
      // set the current world ...
      std::cout << "#osp:qtv: setting world ..." << std::endl;
      if (verbosity >= 1) {
        std::cout << "#osp:qtv: world bounds is " << world->bounds()
                  << std::endl;
      }

      sg::RenderContext ctx;

      renderer->traverse(ctx, "commit");
      renderer->traverse(ctx, "render");

      // -------------------------------------------------------
      // initialize renderer's integrator
      // -------------------------------------------------------
      {
        // first, check if one is specified in the scene file.
        auto integrator = renderer->lastDefinedIntegrator();
        if (!integrator) {
          std::string integratorName = integratorFromCommandLine;
          if (integratorName == "")
            integratorName = DEFAULT_INTEGRATOR_NAME;
          integrator = std::make_shared<sg::Integrator>(integratorName);
        }
        renderer->setIntegrator(integrator);
        integrator->setSPP(spp);
        integrator->commit();
      }

      // -------------------------------------------------------
      // initialize renderer's camera
      // -------------------------------------------------------
      {
        // activate the last camera defined in the scene graph (if set)
        if (cameraFromCommandLine) {
          renderer->setCamera(std::dynamic_pointer_cast<sg::Camera>(cameraFromCommandLine));
        } else {
          renderer->setCamera(renderer->lastDefinedCamera());
        }
        if (!renderer->camera) {
          renderer->setCamera(renderer->createDefaultCamera(upFromCommandLine));
        }
      }

      // -------------------------------------------------------
      // determine output method: offline to file, or interactive viewer
      // -------------------------------------------------------
      if (outFileName == "") {
        // create new modelviewer
        cout << "#osp:qtv: setting up to open QT viewer window" << endl;
        ModelViewer *modelViewer = new ModelViewer(renderer, fullscreen);
        // modelViewer->setFixedSize(frameResolution.x,frameResolution.y);
        modelViewer->showFrameRate(showFPS);
        std::cout << "#osp:qtv: Press 'f' to toggle fullscreen rendering mode" << endl;
        if (fullscreen){
          std::cout << "#osp:qtv: opening fullscreen viewer, press 'ESC' to quit" << endl;
          modelViewer->showFullScreen();
        } else {
          modelViewer->show();
        }

        if (frameResolution.x > 0) {
          cout << "#osp:qtv: trying to set render size to "
               << frameResolution.x << "x" << frameResolution.y << " pixels"
               << endl;
          
          float frame_width  = modelViewer->width()
                               - modelViewer->renderWidget->width();
          float frame_height = modelViewer->height()
                               - modelViewer->renderWidget->height();
          modelViewer->resize(frameResolution.x+frame_width,
                              frameResolution.y+frame_height);
          cout << "#osp:qtv: now rendering at "
               << modelViewer->renderWidget->width()
               << "x" << modelViewer->renderWidget->height() << " pixels."
               << endl;
        }

        // let qt run...
        app->exec();
        
        // done, closing down.
        delete modelViewer;
        delete app;
      } else {
        cout << "#osp:qtv: setting up in render-to-file mode" << endl;
        if (frameResolution == vec2i(-1, -1)) {
          cout << "#osp:qtv: Warning! "
               << "no resolution specified, defaulting to 1280x720" << endl;
          frameResolution = vec2i(1280, 720);
        }
        if (!renderer->frameBuffer) {
          cout << "#osp:qtv: creating default framebuffer (" 
               << frameResolution.x << "x" << frameResolution.y << ")" << endl;
          renderer->frameBuffer =
              std::make_shared<sg::FrameBuffer>(frameResolution);
        }
        renderer->frameBuffer->commit(); 
        renderer->frameBuffer->clear();

        // output file specified - render to file
        cout << "#osp:qtv: rendering frame" << endl;
        renderer->renderFrame();

        unsigned char *fbMem = renderer->frameBuffer->map();
        cout << "#osp:qtv: saving image" << endl;
        // PRINT((int*)fbMem);
        // PRINT(*(int**)fbMem);
        QImage image = QImage(fbMem,
                              renderer->frameBuffer->size().x,
                              renderer->frameBuffer->size().y,
                              QImage::Format_ARGB32).rgbSwapped().mirrored();
        // QImage fb = QImage(fbMem,size.x,size.y,QImage::Format_RGB32).rgbSwapped().mirrored();
        image.save(outFileName.c_str());
        renderer->frameBuffer->unmap(fbMem);
        cout << "#osp:qtv: rendered image saved to " << outFileName << endl;
        cout << "#osp:qtv: done... exiting." << endl;
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
    std::cerr << "#osp:qtv: fatal error: " << e.what() << std::endl;
    return 1;
  }
}
  
