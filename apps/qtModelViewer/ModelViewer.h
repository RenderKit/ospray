/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// viewer
//#include "Scene.h"
// widgets
#include "widgets/affineSpaceManipulator/QAffineSpaceManipulator.h"
#include "widgets/transferFunction/QTransferFunctionEditor.h"

// #include "sg/Scene.h"

namespace ospray {
  namespace viewer {

    struct OSPRayRenderer {
       //      Ref<sg::World> world;

      virtual void render() { PING; };
    };

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

    /*! \brief main QT window of the model viewer */
    struct ModelViewer : public QMainWindow {
      Q_OBJECT
      
    public:
      ModelViewer();
    public slots:
      //! signals that the render widget changed one of the inputs
      //! (most likely, that the camera position got changed)
      void cameraChanged();

      void render();

    protected:
      //! create the lower-side time step slider (for models that have
      //! time steps; won't do anything for models that don't)
      void createTimeSlider();
      //! create the widet on the side that'll host all the editing widgets
      void createEditorWidgetStack();
      //! create lights editor widget
      void createLightsEditor();
      //! create a transfer fucntion editor
      void createTransferFunctionEditor();

      //! the ospray renderer we're using to render our frame
      OSPRayRenderer *renderer;

      // -------------------------------------------------------
      // qt gui components 
      // -------------------------------------------------------
      EditorWidgetStack        *editorWidgetStack;
      QToolBar                 *toolBar;
      QAffineSpaceManipulator  *renderWidget;
      QTransferFunctionEditor  *transferFunctionEditor;
    };
  }
}

