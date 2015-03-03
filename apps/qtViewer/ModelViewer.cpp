// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "ModelViewer.h"
#include "widgets/affineSpaceManipulator/HelperGeometry.h"

#include "sg/SceneGraph.h"
#include "sg/Renderer.h"

namespace ospray {
  namespace viewer {

    using std::cout;
    using std::endl;

    struct FPSCounter {
      double smooth_nom;
      double smooth_den;
      double frameStartTime;

      FPSCounter() 
      {
        smooth_nom = 0.;
        smooth_den = 0.;
        frameStartTime = 0.;
      }
      void startRender() { frameStartTime = ospray::getSysTime(); }
      void doneRender() { 
        double seconds = ospray::getSysTime() - frameStartTime; 
        smooth_nom = smooth_nom * 0.8f + seconds;
        smooth_den = smooth_den * 0.8f + 1.f;
      }
      double getFPS() const { return smooth_den / smooth_nom; }
    };

    FPSCounter fps;

    OSPRayRenderWidget::OSPRayRenderWidget(Ref<sg::Renderer> renderer)
      : QAffineSpaceManipulator(QAffineSpaceManipulator::INSPECT),
        sgRenderer(renderer)
    {
      if (renderer->world) {
        box3f worldBounds = renderer->world->getBounds();
        if (!worldBounds.empty()) {
          float moveSpeed = .25*length(worldBounds.size());
          QAffineSpaceManipulator::setMoveSpeed(moveSpeed);
        }
      }
      Ref<sg::PerspectiveCamera> camera = renderer->camera.cast<sg::PerspectiveCamera>();
      if (camera) {
        frame->sourcePoint = camera->getFrom();
        frame->targetPoint = camera->getAt();
        frame->upVector    = camera->getUp();
        frame->orientation.vz = normalize(camera->getUp());
        frame->orientation.vy = normalize(camera->getAt() - camera->getFrom());
        frame->orientation.vx = normalize(cross(frame->orientation.vy,frame->orientation.vz));
      }
    }
    
    void OSPRayRenderWidget::setWorld(Ref<sg::World> world)
    {
      assert(sgRenderer);
      sgRenderer->setWorld(world);
      cout << "#ospQTV: world set, found " 
           << sgRenderer->allNodes.size() << " nodes" << endl;
   }

    //! the QT callback that tells us that we have to redraw
    void OSPRayRenderWidget::redraw()
    { 
      if (!sgRenderer) return;
      if (!sgRenderer->frameBuffer) return;
      if (!sgRenderer->camera) return;

      if (showFPS) {
        static int frameID = 0;
        if (frameID > 0) {
          fps.doneRender();
          printf("fps: %f\n",fps.getFPS());
        }
        fps.startRender();
        ++frameID;
      }

      sgRenderer->renderFrame();


      vec2i size = sgRenderer->frameBuffer->getSize();
      unsigned char *fbMem = sgRenderer->frameBuffer->map();
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, fbMem);      
      sgRenderer->frameBuffer->unmap(fbMem);

      // accumulate, but only up to 32 frames
      if (sgRenderer->accumID < 32)
        update();
    }
    
    //! the QT callback that tells us that the image got resize
    void OSPRayRenderWidget::resize(int width, int height)
    { 
      sgRenderer->frameBuffer = new sg::FrameBuffer(vec2i(width,height));
      sgRenderer->resetAccumulation();
    }

    //! update the ospray camera (ospCamera) from the widget camera (this->camera)
    void OSPRayRenderWidget::updateOSPRayCamera()
    {
      if (!sgRenderer) return;
      if (!sgRenderer->camera) return;

      sgRenderer->resetAccumulation();
      Ref<sg::PerspectiveCamera> camera = sgRenderer->camera.cast<sg::PerspectiveCamera>();
      assert(camera);
      const vec3f from = frame->sourcePoint;
      const vec3f at   = frame->targetPoint;
      vec2i size = sgRenderer->frameBuffer->getSize();

      camera->setFrom(from);
      camera->setAt(at);
      camera->setUp(frame->orientation.vz);
      camera->setAspect(size.x/float(size.y));

      camera->commit();
    }

    //! create the lower-side time step slider (for models that have
    //! time steps; won't do anything for models that don't)
    void ModelViewer::createTimeSlider()
    {
    }

    //! create the widet on the side that'll host all the editing widgets
    void ModelViewer::createEditorWidgetStack()
    {
      editorWidgetStack = new EditorWidgetStack;
      // editorWidgetStack->addPage("Test1",new QLabel("Test1"));
      // editorWidgetStack->addPage("Test2",new QLabel("Test2"));
      QDockWidget *dock = new QDockWidget(this);
      dock->setWindowTitle("Editors");
      dock->setWidget(editorWidgetStack);
      dock->setFeatures(0);
      addDockWidget(Qt::RightDockWidgetArea,dock);
    }

    void ModelViewer::createTransferFunctionEditor()
    {
      QWidget *xfEditorsPage = NULL;
      // make a list of all transfer function nodes in the scene graph
      std::vector<Ref<sg::TransferFunction> > xferFuncs;
      for (int i=0;i<sgRenderer->uniqueNodes.size();i++) {
        sg::TransferFunction *xf = dynamic_cast<sg::TransferFunction *>
          (sgRenderer->uniqueNodes.object[i]->node.ptr);
        if (xf) xferFuncs.push_back(xf);
      }
      std::cout << "#osp:qtv: found " << xferFuncs.size() 
                << " transfer function nodes" << std::endl;

      if (xferFuncs.empty()) {
        xfEditorsPage = new QLabel("(no xfer fcts found)");
      } else {
        // -------------------------------------------------------
        // found some transfer functions - create a stacked widget
        // with an editor for each
        // -------------------------------------------------------
        xfEditorsPage = new QWidget;
        QVBoxLayout    *layout = new QVBoxLayout;
        xfEditorsPage->setLayout(layout);


        QStackedWidget *stackedWidget = new QStackedWidget;
        QComboBox *pageComboBox = new QComboBox;
        QObject::connect(pageComboBox, SIGNAL(activated(int)), 
                         stackedWidget, SLOT(setCurrentIndex(int)));

        layout->addWidget(pageComboBox);
        layout->addWidget(stackedWidget);        

        // now, create widgets for all of them
        for (int i=0;i<xferFuncs.size();i++) {
          // take name from node, or create one
          std::string name = xferFuncs[i]->name;
          if (name == "") {
            std::stringstream ss;
            ss << "(unnamed xfr fct #" << i << ")";
            name = ss.str();
          }
          // add combo box and stacked widget entries
          pageComboBox->addItem(tr(name.c_str()));

          // create a transfer function editor for this transfer function node
          QOSPTransferFunctionEditor *xfEd 
            = new QOSPTransferFunctionEditor(xferFuncs[i]);
          stackedWidget->addWidget(xfEd);
          connect(xfEd, SIGNAL(transferFunctionChanged()), 
                  this, SLOT(render()));
        }
      }
      editorWidgetStack->addPage("Transfer Functions",xfEditorsPage);
    }

	void ModelViewer::keyPressEvent(QKeyEvent *event){
		if (event->key() == Qt::Key_Escape){
			// TODO: Properly tell the app to quit?
			exit(0);
		} else {
			QMainWindow::keyPressEvent(event);
		}
	}

    ModelViewer::ModelViewer(Ref<sg::Renderer> sgRenderer, bool fullscreen)
      : editorWidgetStack(NULL),
        transferFunctionEditor(NULL),
        toolBar(NULL),
        sgRenderer(sgRenderer)
    {
      // resize to default window size
      setWindowTitle(tr("OSPRay QT ModelViewer"));
      resize(1024,768);

	  if (!fullscreen){
		  // create GUI elements
		  toolBar      = addToolBar("toolbar");

		  QAction *printCameraAction = new QAction("Print Camera", this);
		  connect(printCameraAction, SIGNAL(triggered()), this, SLOT(printCameraAction()));
		  toolBar->addAction(printCameraAction);

		  QAction *screenShotAction = new QAction("Screenshot", this);
		  connect(screenShotAction, SIGNAL(triggered()), this, SLOT(screenShotAction()));
		  toolBar->addAction(screenShotAction);
	  }

      renderWidget = new OSPRayRenderWidget(sgRenderer);
      ///renderWidget = new CheckeredSphereRotationEditor();
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FLY);
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::INSPECT);
      // renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FREE_ROTATION);
      //      renderWidget->setMoveSpeed(1.f);
      connect(renderWidget,SIGNAL(affineSpaceChanged(QAffineSpaceManipulator *)),
              this,SLOT(cameraChanged()));

      setCentralWidget(renderWidget);

	  if (!fullscreen){
		  createTimeSlider();
		  createEditorWidgetStack();
		  createTransferFunctionEditor();
	  }
    }

    void ModelViewer::setWorld(Ref<sg::World> world) 
    {
      renderWidget->setWorld(world); 
    }

    void ModelViewer::render() { 
      sgRenderer->resetAccumulation();
      if (renderWidget) renderWidget->updateGL(); 
    }

    // this is a incoming signal that the render widget changed the camera
    void ModelViewer::cameraChanged() 
    {
      renderWidget->updateOSPRayCamera();
    }

    //! print the camera on the command line (triggered by toolbar/menu).
    void ModelViewer::printCameraAction()
    {
      Ref<sg::PerspectiveCamera> camera = renderWidget->sgRenderer->camera.cast<sg::PerspectiveCamera>();
      vec3f from = camera->getFrom();
      vec3f up   = camera->getUp();
      vec3f at   = camera->getAt();
      std::cout << "#osp:qtv: camera is"
                << " -vp " << from.x << " " << from.y << " " << from.z
                << " -vi " << at.x << " " << at.y << " " << at.z
                << " -vu " << up.x << " " << up.y << " " << up.z
                << std::endl;
    }

    //! take a screen shot
    void ModelViewer::screenShotAction()
    {
      vec2i size = sgRenderer->frameBuffer->getSize();
      unsigned char *fbMem = sgRenderer->frameBuffer->map();
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, fbMem);      
      sgRenderer->frameBuffer->unmap(fbMem);
      
      QImage fb = QImage(fbMem,size.x,size.y,QImage::Format_RGB32).rgbSwapped().mirrored();
      const std::string fileName = "/tmp/ospQTV.screenshot.png";
      fb.save(fileName.c_str());
      std::cout << "screen shot saved in " << fileName << std::endl;
    }
  }
}
