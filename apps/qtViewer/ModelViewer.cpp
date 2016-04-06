// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "FPSCounter.h"
// sg
#include "sg/SceneGraph.h"
#include "sg/Renderer.h"
// std
#include <sstream>

namespace ospray {
  namespace viewer {

    using std::cout;
    using std::endl;

    float autoRotateSpeed = 0.f;

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
          printf("fps: %7.3f (smoothed: %7.3f)\n",fps.getFPS(),fps.getSmoothFPS());
        }
        fps.startRender();
        ++frameID;
      }

      sgRenderer->renderFrame();


      vec2i size = sgRenderer->frameBuffer->getSize();
      unsigned char *fbMem = sgRenderer->frameBuffer->map();
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, fbMem);
      sgRenderer->frameBuffer->unmap(fbMem);

      if (autoRotateSpeed != 0.f) {
        rotateAroundTarget(autoRotateSpeed,0.f);
        // sgRenderer->resetAccumulation();
        updateOSPRayCamera();
        update();
      }
      // accumulate, but only up to 32 frames
      if (sgRenderer->accumID < 32) {
        update();
      }
    }

    //! the QT callback that tells us that the image got resize
    void OSPRayRenderWidget::resize(int width, int height)
    {
      sgRenderer->frameBuffer = new sg::FrameBuffer(vec2i(width,height));
      sgRenderer->resetAccumulation();

      Ref<sg::PerspectiveCamera> camera = sgRenderer->camera.cast<sg::PerspectiveCamera>();
      camera->setAspect(width/float(height));
      camera->commit();
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
      editorWidgetDock = new QDockWidget(this);
      editorWidgetDock->setWindowTitle("Editors");
      editorWidgetDock->setWidget(editorWidgetStack);
      editorWidgetDock->setFeatures(0);
      addDockWidget(Qt::RightDockWidgetArea,editorWidgetDock);
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

          TransferFunction *xf = xferFuncs[i].ptr;
          assert(xf);
          // create a transfer function editor for this transfer function node
          QOSPTransferFunctionEditor *xfEd
            = new QOSPTransferFunctionEditor(xf);
          const std::vector<std::pair<float,float> > &alpha = xf->getAlphaArray();
          if (!alpha.empty()) {
            std::vector<ospcommon::vec2f> points;
            for (int i=0;i<alpha.size();i++)
              points.push_back(vec2f(alpha[i].first,alpha[i].second));
            xfEd->setOpacityPoints(points);
          }
          stackedWidget->addWidget(xfEd);
          connect(xfEd, SIGNAL(transferFunctionChanged()),
                  this, SLOT(render()));
        }
      }
      editorWidgetStack->addPage("Transfer Functions",xfEditorsPage);
    }

    void ModelViewer::createLightManipulator()
    {
      QWidget *lmEditorsPage = new QWidget;
      QVBoxLayout *layout = new QVBoxLayout;
      lmEditorsPage->setLayout(layout);

      QStackedWidget *stackedWidget = new QStackedWidget;
      layout->addWidget(stackedWidget);

      Ref<sg::PerspectiveCamera> camera = renderWidget->sgRenderer->camera.cast<sg::PerspectiveCamera>();
      QLightManipulator *lManipulator = new QLightManipulator(sgRenderer, camera->getUp());
      //stackedWidget->addWidget(lManipulator);
      layout->addWidget(lManipulator);

      editorWidgetStack->addPage("Light Editor", lManipulator);

      connect(lManipulator, SIGNAL(lightsChanged()), this, SLOT(render()));
    }

    void ModelViewer::toggleUpAxis(int axis)
    {
      std::cout << "#osp:QTV: new upvector is " << renderWidget->getFrame()->upVector << std::endl;
    }

    void ModelViewer::keyPressEvent(QKeyEvent *event) {
      switch (event->key()) {
      case Qt::Key_Escape: {
        QApplication::quit();
      } break;
      case '{': {
        if (autoRotateSpeed == 0.f) {
          autoRotateSpeed = -1.f;
        } else if (autoRotateSpeed < 0) {
          autoRotateSpeed *= 1.5f;
        } else {
          autoRotateSpeed /= 1.5f;
          if (autoRotateSpeed < 1.f)
            autoRotateSpeed = 0.f;
        }
        printf("new auto-rotate speed %f\n",autoRotateSpeed);
        update();
      } break;
      case '}': {
        if (autoRotateSpeed == 0.f) {
          autoRotateSpeed = +1.f;
        } else if (autoRotateSpeed < 0) {
          autoRotateSpeed /= 1.5f;
          if (autoRotateSpeed > -1.f)
            autoRotateSpeed = 0.f;
        } else {
          autoRotateSpeed *= 1.5f;
        }
        printf("new auto-rotate speed %f\n",autoRotateSpeed);
        update();
      } break;
      case '|': {
        printf("'|' key - stopping auto-rotation\n");
        autoRotateSpeed = 0.f;
        update();
      } break;
      case Qt::Key_C: {
        // ------------------------------------------------------------------
        // 'C':
        // - Shift-C print current camera
        // ------------------------------------------------------------------
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-f: switch to fly mode
          std::cout << "Shift-C: Printing camera" << std::endl;
          printCameraAction();
        }
      } break;
      case Qt::Key_F: {
        // ------------------------------------------------------------------
        // 'F':
        // - Shift-F enters fly mode
        // - Ctrl-F toggles full-screen
        // ------------------------------------------------------------------
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-f: switch to fly mode
          std::cout << "Shift-F: Entering 'Fly' mode" << std::endl;
          renderWidget->setInteractionMode(QAffineSpaceManipulator::FLY);
        } else if (event->modifiers() & Qt::ControlModifier) {
          // ctrl-f: switch to full-screen
          std::cout << "Ctrl-F: Toggling full-screen mode" << std::endl;
          setWindowState(windowState() ^ Qt::WindowFullScreen);
          if (windowState() & Qt::WindowFullScreen){
            toolBar->hide();
            editorWidgetDock->hide();
          } else {
            toolBar->show();
            editorWidgetDock->show();
          }
        }
      } break;
      case Qt::Key_I: {
        // ------------------------------------------------------------------
        // 'I':
        // - Ctrl-I switches to inspect mode
        // ------------------------------------------------------------------
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-f: switch to fly mode
          std::cout << "Shift-I: Entering 'Inspect' mode" << std::endl;
          renderWidget->setInteractionMode(QAffineSpaceManipulator::INSPECT);
        }
      } break;
      case Qt::Key_Q: {
        QApplication::quit();
      } break;
      case Qt::Key_R: {
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-f: switch to fly mode
          std::cout << "Shift-R: Entering Free-'Rotation' mode (no up-vector)" << std::endl;
          renderWidget->setInteractionMode(QAffineSpaceManipulator::FREE_ROTATION);
        }
      } break;
      case Qt::Key_X: {
        // ------------------------------------------------------------------
        // 'X':
        // - Ctrl-X switches to X-up/down for upvector
        // ------------------------------------------------------------------
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-x: switch to X-up
          std::cout << "Shift-X: switching to X-up/down upvector " << std::endl;
          renderWidget->toggleUp(0);
        }
      } break;
        // ------------------------------------------------------------------
        // 'Y':
        // - Ctrl-Y switches to Y-up/down for upvector
        // ------------------------------------------------------------------
      case Qt::Key_Y: {
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-x: switch to X-up
          std::cout << "Shift-Y: switching to Y-up/down upvector " << std::endl;
          renderWidget->toggleUp(1);
        }
      } break;
      case Qt::Key_Z: {
        // ------------------------------------------------------------------
        // 'Z':
        // - Ctrl-Z switches to Z-up/down for upvector
        // ------------------------------------------------------------------
        if (event->modifiers() & Qt::ShiftModifier) {
          // shift-x: switch to X-up
          std::cout << "Shift-Z: switching to Z-up/down upvector " << std::endl;
          renderWidget->toggleUp(2);
        }
      } break;

      default:
        QMainWindow::keyPressEvent(event);
      }
    }

    ModelViewer::ModelViewer(Ref<sg::Renderer> sgRenderer, bool fullscreen)
      : editorWidgetStack(NULL),
        transferFunctionEditor(NULL),
        lightEditor(NULL),
        toolBar(NULL),
        sgRenderer(sgRenderer)
    {
      // resize to default window size
      setWindowTitle(tr("OSPRay Qt ModelViewer"));
      resize(1024,768);

      // create GUI elements
      toolBar = addToolBar("toolbar");

      QAction *printCameraAction = new QAction("Print Camera", this);
      connect(printCameraAction, SIGNAL(triggered()), this, SLOT(printCameraAction()));
      toolBar->addAction(printCameraAction);

      QAction *screenShotAction = new QAction("Screenshot", this);
      connect(screenShotAction, SIGNAL(triggered()), this, SLOT(screenShotAction()));
      toolBar->addAction(screenShotAction);

      renderWidget = new OSPRayRenderWidget(sgRenderer);
      connect(renderWidget,SIGNAL(affineSpaceChanged(QAffineSpaceManipulator *)),
              this,SLOT(cameraChanged()));

      setCentralWidget(renderWidget);

      setFocusPolicy(Qt::StrongFocus);

      createTimeSlider();
      createEditorWidgetStack();
      createTransferFunctionEditor();
      createLightManipulator();

      if (fullscreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        toolBar->hide();
        editorWidgetDock->hide();
      }
    }

    void ModelViewer::setWorld(Ref<sg::World> world)
    {
      renderWidget->setWorld(world);
    }

    void ModelViewer::render()
    {
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

    void ModelViewer::lightChanged()
    {
    }

  } // ::ospray::viewer
} // ::ospray
