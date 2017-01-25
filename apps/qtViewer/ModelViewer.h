// ======================================================================== //
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

#pragma once

#define WARN_ON_INCLUDING_OSPCOMMON 1

// widgets
#include "widgets/affineSpaceManipulator/QAffineSpaceManipulator.h"
#include "widgets/transferFunction/QTransferFunctionEditor.h"
#include "widgets/lightManipulator/QLightManipulator.h"
// scene graph
#include "sg/Renderer.h"

namespace ospray {
  namespace viewer {

    //! single widget that realizes the stack of editor widgets on the
    //! right of the window. contains a set of individual editors, all
    //! maintained in a stack
    struct EditorWidgetStack : public QWidget {
      
      EditorWidgetStack()
      {
        pageComboBox  = new QComboBox;
        stackedWidget = new QStackedWidget;
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(pageComboBox);
        layout->addWidget(stackedWidget);
        setLayout(layout);
      }
      
      void addPage(const std::string &name, QWidget *widget)
      {
        assert(currentPages.find(name) == currentPages.end());
        currentPages[name] = widget;
        stackedWidget->addWidget(widget);
        pageComboBox->addItem(tr(name.c_str()));
        connect(pageComboBox,SIGNAL(activated(int)),stackedWidget,SLOT(setCurrentIndex(int)));
      }

    protected:
      std::map<std::string,QWidget *> currentPages;
      QComboBox *pageComboBox;
      QStackedWidget *stackedWidget;
    };


    // =======================================================
    //! render widget that renders through ospray
    // =======================================================
    struct OSPRayRenderWidget : public QAffineSpaceManipulator {
      OSPRayRenderWidget(std::shared_ptr<sg::Renderer> renderer);

      std::shared_ptr<sg::Renderer> sgRenderer;

      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      // std::shared_ptr<sg::Renderer> sgRenderer;

      virtual void redraw();
      virtual void resize(int width, int height);

      void setWorld(std::shared_ptr<sg::World> world);

      // -------------------------------------------------------
      // internal state
      // -------------------------------------------------------

      //! update the ospray camera (ospCamera) from the widget camera (this->camera)
      void updateOSPRayCamera();

      //! signals that the current ospray state is dirty (i.e., that
      //! we can not accumulate)
      bool isDirty;

      //! the world we're displaying
      std::shared_ptr<sg::World> world;

      //! whether to display the frame rate
      bool showFPS;
    };


    /*! \brief main QT window of the model viewer */
    struct ModelViewer : public QMainWindow {
      Q_OBJECT

    public:
      ModelViewer(std::shared_ptr<sg::Renderer> sgRenderer, bool fullscreen);

      void toggleUpAxis(int axis);

    public slots:
      //! signals that the render widget changed one of the inputs
      //! (most likely, that the camera position got changed)
      void cameraChanged();

      //! print the camera on the command line (triggered by toolbar/menu).
      void printCameraAction();
      //! take a screen shot
      void screenShotAction();

      void render();

      void setWorld(std::shared_ptr<sg::World> newWorld);

      /*! enable/disable display of frame rate */
      void showFrameRate(bool showFPS) { this->renderWidget->showFPS = showFPS; }

      //! Catches that the light manipulator has changed our default light
      void lightChanged();

    protected:
        //! create the lower-side time step slider (for models that have
        //! time steps; won't do anything for models that don't)
        void createTimeSlider();
        //! create the widet on the side that'll host all the editing widgets
        void createEditorWidgetStack();
        //! create a transfer fucntion editor
        void createTransferFunctionEditor();
        //! create a light manipulator
        void createLightManipulator();
        //! use escape to quit
        virtual void keyPressEvent(QKeyEvent *event);

        // -------------------------------------------------------
        // qt gui components
        // -------------------------------------------------------
        EditorWidgetStack        *editorWidgetStack;
        QDockWidget              *editorWidgetDock;
        QToolBar                 *toolBar;
        QTransferFunctionEditor  *transferFunctionEditor;
        QLightManipulator        *lightEditor;

        std::shared_ptr<sg::Renderer> sgRenderer;

    public:
      OSPRayRenderWidget       *renderWidget;
    };
  }
}

