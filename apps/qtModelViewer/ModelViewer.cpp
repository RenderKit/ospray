/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ModelViewer.h"
#include "widgets/affineSpaceManipulator/HelperGeometry.h"

#include "sg/SceneGraph.h"
#include "sg/Renderer.h"

namespace ospray {
  namespace viewer {

    using std::cout;
    using std::endl;

    OSPRayRenderWidget::OSPRayRenderWidget(Ref<sg::Renderer> renderer)
      : QAffineSpaceManipulator(QAffineSpaceManipulator::INSPECT),
        sgRenderer(renderer)
    {
      box3f worldBounds = renderer->world->getBounds();
      if (!worldBounds.empty()) {
        QAffineSpaceManipulator::setMoveSpeed(length(worldBounds.size())*1e-3f);
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

#if 0
      // re-use that code for TriangleMesh-node code:
      CoordFrameGeometry arrows;

      ospModel = ospNewModel();
      assert(ospModel);

      const char *rendererType = "eyeLight_vertexColor";

      for (int axisID=0;axisID<3;axisID++) {
        HelperGeometry::Mesh &mesh = arrows.arrow[axisID];
        
        std::vector<vec3fa> color;
        for (int i=0;i<mesh.vertex.size();i++)
          color.push_back(mesh.color);
        OSPData coordData  = ospNewData(mesh.vertex.size(),OSP_FLOAT3A,&mesh.vertex[0]);
        OSPData normalData = ospNewData(mesh.normal.size(),OSP_FLOAT3A,&mesh.normal[0]);
        OSPData colorData  = ospNewData(color.size(),OSP_FLOAT3A,&color[0]);
        OSPData indexData  = ospNewData(mesh.index.size(),OSP_INT3,&mesh.index[0]);
        OSPGeometry triMesh = ospNewTriangleMesh();
        assert(triMesh);
        ospSetData(triMesh,"vertex",coordData);
        ospSetData(triMesh,"vertex.normal",normalData);
        ospSetData(triMesh,"vertex.color",colorData);
        ospSetData(triMesh,"index",indexData);
        ospCommit(triMesh);
        ospAddGeometry(ospModel,triMesh);
      }
      ospCommit(ospModel);
      
#endif
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

      updateOSPRayCamera();
      sgRenderer->renderFrame();

      vec2i size = sgRenderer->frameBuffer->getSize();
      unsigned char *fbMem = sgRenderer->frameBuffer->map();
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, fbMem);      
      sgRenderer->frameBuffer->unmap(fbMem);
      
      // updateOSPRayCamera();
    }
    
    //! the QT callback that tells us that the image got resize
    void OSPRayRenderWidget::resize(int width, int height)
    { 
      sgRenderer->frameBuffer = new sg::FrameBuffer(vec2i(width,height));
      sgRenderer->resetAccumulation();
      // if (ospFrameBuffer)
      //   ospFreeFrameBuffer(ospFrameBuffer);
      // ospFrameBuffer = ospNewFrameBuffer(size,OSP_RGBA_I8);
      // assert(ospFrameBuffer);
    }

    //! update the ospray camera (ospCamera) from the widget camera (this->camera)
    void OSPRayRenderWidget::updateOSPRayCamera()
    {
      if (!sgRenderer) return;
      if (!sgRenderer->camera) return;

#if 1
      Ref<sg::PerspectiveCamera> camera = sgRenderer->camera.cast<sg::PerspectiveCamera>();
      assert(camera);
      const vec3f from = frame->sourcePoint;
      const vec3f at   = frame->targetPoint;

      camera->setFrom(from);
      camera->setAt(at);
      camera->setUp(frame->orientation.vz);
      camera->commit();
#else
      OSPCamera ospCamera = sgRenderer->camera->ospCamera;

      ospSetVec3f(ospCamera,"pos",from);
      ospSetVec3f(ospCamera,"dir",at - from);
      ospSetVec3f(ospCamera,"up",frame->orientation.vz);
      ospSetf(ospCamera,"aspect",size.x/float(size.y));
      ospCommit(ospCamera);      
#endif
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
#if 1
          QOSPTransferFunctionEditor *xfEd 
            = new QOSPTransferFunctionEditor(xferFuncs[i]);
          stackedWidget->addWidget(xfEd);
          connect(xfEd, SIGNAL(transferFunctionChanged()), 
                  this, SLOT(render()));
#else
          stackedWidget->addWidget(new QOSPTransferFunctionEditor(xferFuncs[i]));
#endif

    
        }
      }
      editorWidgetStack->addPage("Transfer Functions",xfEditorsPage);
    }

    ModelViewer::ModelViewer(Ref<sg::Renderer> sgRenderer)
      : editorWidgetStack(NULL),
        transferFunctionEditor(NULL),
        toolBar(NULL),
        sgRenderer(sgRenderer)
    {
      // resize to default window size
      //resize(320, 240);
      setWindowTitle(tr("OSPRay QT ModelViewer"));
      resize(1024,768);

      // initialize renderer
      // renderer     = new OSPRayRenderer;

      // create GUI elements
      toolBar      = addToolBar("toolbar");
      QAction *testAction = new QAction("test", this);
      //connect(nextTimestepAction, SIGNAL(triggered()), this, SLOT(nextTimestep()));
      toolBar->addAction(testAction);

      renderWidget = new OSPRayRenderWidget(sgRenderer);
      ///renderWidget = new CheckeredSphereRotationEditor();
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FLY);
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::INSPECT);
      // renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FREE_ROTATION);
      renderWidget->setMoveSpeed(1.f);
      connect(renderWidget,SIGNAL(affineSpaceChanged(QAffineSpaceManipulator *)),
              this,SLOT(cameraChanged()));

      setCentralWidget(renderWidget);

      createTimeSlider();

      createEditorWidgetStack();
      createTransferFunctionEditor();
    }

    void ModelViewer::setWorld(Ref<sg::World> world) 
    {
      renderWidget->setWorld(world); 
    }

    void ModelViewer::render() { 
      sgRenderer->resetAccumulation();
      if (renderWidget) renderWidget->updateGL(); 
    }

    void ModelViewer::cameraChanged() 
    {
      /* nothing to do here, yet - the ospray render widget
         automatically updates the camera used for rendering. We can
         still add some callbacks here eventually, but right now we
         don't need this callback */
    }
  }
}
