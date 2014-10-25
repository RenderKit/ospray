/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ModelViewer.h"
#include "widgets/affineSpaceManipulator/HelperGeometry.h"

//#include "sg/Scene.h"

namespace ospray {
  namespace viewer {

        // =======================================================
    //! render widget that renders through ospray
    // =======================================================
    struct OSPRayRenderWidget : public QAffineSpaceManipulator {
      OSPRayRenderWidget();

      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      virtual void redraw();
      virtual void resize(int width, int height);

      // -------------------------------------------------------
      // internal state
      // -------------------------------------------------------

      //! the ospray frame buffer we're using
      OSPFrameBuffer ospFrameBuffer;

      //! the renderer we're using
      OSPRenderer    ospRenderer;
      
      //! the world model that we're using to render
      OSPModel       ospModel; 

      //! ospray camera we're using to render with
      OSPCamera      ospCamera;

      //! update the ospray camera (ospCamera) from the widget camera (this->camera)
      void updateOSPRayCamera();

      //! signals that the current ospray state is dirty (i.e., that we can not accumulate)
      bool isDirty; 
    };

    OSPRayRenderWidget::OSPRayRenderWidget()
      : QAffineSpaceManipulator(QAffineSpaceManipulator::INSPECT),
        ospRenderer(NULL), ospModel(NULL), ospCamera(NULL), ospFrameBuffer(NULL)
    {
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
      
      ospCamera = ospNewCamera("perspective");
      updateOSPRayCamera();
      
      ospRenderer = ospNewRenderer(rendererType);
      assert(ospRenderer);
      if (!ospRenderer)
        throw std::runtime_error("#ospQTV: could not create renderer");

      ospSetObject(ospRenderer,"world",ospModel);
      ospSetObject(ospRenderer,"camera",ospCamera);
      ospCommit(ospRenderer);
    }
    
    //! the QT callback that tells us that we have to redraw
    void OSPRayRenderWidget::redraw()
    { 
      if (!ospFrameBuffer) return;
      if (!ospRenderer) return;
      
      updateOSPRayCamera();
      ospRenderFrame(ospFrameBuffer, ospRenderer);
      
      uint32 *mappedFB = (unsigned int *)ospMapFrameBuffer(ospFrameBuffer);      
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, mappedFB);      
      ospUnmapFrameBuffer(mappedFB, ospFrameBuffer);
    }
    
    //! the QT callback that tells us that the image got resize
    void OSPRayRenderWidget::resize(int width, int height)
    { 
      if (ospFrameBuffer)
        ospFreeFrameBuffer(ospFrameBuffer);
      ospFrameBuffer = ospNewFrameBuffer(size,OSP_RGBA_I8);
      assert(ospFrameBuffer);
    }

    //! update the ospray camera (ospCamera) from the widget camera (this->camera)
    void OSPRayRenderWidget::updateOSPRayCamera()
    {
      assert(frame);
      assert(ospCamera);

      const vec3f from = frame->sourcePoint;
      const vec3f at   = frame->targetPoint;

      ospSetVec3f(ospCamera,"pos",from);
      ospSetVec3f(ospCamera,"dir",at - from);
      ospSetVec3f(ospCamera,"up",frame->orientation.vz);
      ospSetf(ospCamera,"aspect",size.x/float(size.y));
      ospCommit(ospCamera);      
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
      editorWidgetStack->addPage("Test1",new QLabel("Test1"));
      editorWidgetStack->addPage("Test2",new QLabel("Test2"));
      QDockWidget *dock = new QDockWidget(this);
      dock->setWindowTitle("Editors");
      dock->setWidget(editorWidgetStack);
      dock->setFeatures(0);
      addDockWidget(Qt::RightDockWidgetArea,dock);
    }


    void ModelViewer::createTransferFunctionEditor()
    {
      assert(transferFunctionEditor == NULL);
      transferFunctionEditor = new QTransferFunctionEditor;
      editorWidgetStack->addPage("Transfer Function",transferFunctionEditor);
    }

    ModelViewer::ModelViewer()
      : editorWidgetStack(NULL),
        transferFunctionEditor(NULL),
        toolBar(NULL)
    {
      // resize to default window size
      //resize(320, 240);
      setWindowTitle(tr("OSPRay QT ModelViewer"));
      resize(1024,768);

      // initialize renderer
      renderer     = new OSPRayRenderer;

      // create GUI elements
      toolBar      = addToolBar("toolbar");
      QAction *testAction = new QAction("test", this);
      //connect(nextTimestepAction, SIGNAL(triggered()), this, SLOT(nextTimestep()));
      toolBar->addAction(testAction);

      renderWidget = new OSPRayRenderWidget();
      ///renderWidget = new CheckeredSphereRotationEditor();
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FLY);
      //      renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::INSPECT);
      // renderWidget = new QCoordAxisFrameEditor(QAffineSpaceManipulator::FREE_ROTATION);
      renderWidget->setMoveSpeed(1.f);
      connect(renderWidget,SIGNAL(cameraChanged()),this,SLOT(cameraChanged()));

      setCentralWidget(renderWidget);

      createTimeSlider();

      createEditorWidgetStack();
      createTransferFunctionEditor();
    }

    void ModelViewer::render()
    {
      PING;
    }

    void ModelViewer::cameraChanged() 
    { PING; }
  }
}
