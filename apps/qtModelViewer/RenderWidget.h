/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray interna
#include "ospray/common/ospcommon.h"
// ospray public api
#include "ospray/ospray.h"
// qt
#include <QtGui>
#include <QGLWidget>

namespace ospray {
  namespace viewer {
    // =======================================================
    //! \brief A QT Widget that allows mouse manipulation for a camera 
    // =======================================================
    struct RotationWidget : public QGLWidget {

      //! camera manipulation mode we're in (determines how mouse motion influences the camera pose)
        typedef enum { 
          //! mode in which the camera emulates a person 'flying' through the scene
          FLY, 
          
          //! mode in which the camera rotates ardound the center of interest (the lookat)
          FREE_ROTATION,

          //! mode in which the camera rotates ardound the center of
          //! interest (the lookat), but where the user can also
          //! change the center of inspection
          INSPECT,
        } InteractionMode;

      //! camera class the the user can modify with mouse motion
      struct Camera {

        Camera();

        //! where viewer is located
        vec3f from;
        //! point viewer is looking *at*; also the point many rotations will rotate around
        vec3f at;
        //! look-up vector; can be '(0,0,0)' to indicate that no lookup'ing will be done.
        vec3f up; 
        //! camera coordinate frame (normalized)
        linear3f frame;
        //! last time (in 'times()' that this camera got updatesd)
        size_t lastModified; 
      };

    signals:
      //! the signal we're emitting to tell our listener(s) that something has changed
      void cameraChanged(RotationWidget *which);
      
    public:
      //! constructor
      RotationWidget();
      
      //! set camera to a default pose that overlooks the given bounding box
      void setDefaultCamera(const box3f &worldBounds);

      //! return the camera
      const Camera *getCamera() const { return camera; }
      
      //! return size of widget in pixels
      inline vec2i getSize() const { return size; }

      virtual void render() { PING; }

      // -------------------------------------------------------
      // QT callbacks
      // -------------------------------------------------------

      //! the QT callback that tells us that we have to redraw
      virtual void paintGL();
      //! the QT callback that tells us that the image got resize
      virtual void resizeGL(int width, int height);
      virtual void mousePressEvent(QMouseEvent * event);
      virtual void mouseReleaseEvent(QMouseEvent * event);
      virtual void mouseMoveEvent(QMouseEvent * event);


      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      virtual void redraw() {};
      virtual void resize(int width, int height) {};


      // -------------------------------------------------------
      // overloadable callbacks
      // -------------------------------------------------------
      virtual void pick(QMouseEvent *event) 
      { std::cout << "pcking at " << event->pos().x() << "." << event->pos().y() << std::endl; }
    protected:
      //! size of window, in pixels
      vec2i         size; 
      //! viewport the user is modifying
      Camera       *camera;
      //! camera manipulation mode we're in (determines how mouse motion influences the camera pose)
      InteractionMode interactionMode;

      QPoint lastMousePos;
      size_t startDragTime;
      QPoint startDragPos;

      //! for auto-spinning: last rotation matrix
      float lastRotationAngularSpeed;
      vec3f lastRotationAxis;

    };

    // =======================================================
    //! a widget that allows for rotating a checkered sphere
    // =======================================================
    struct CheckeredSphereRotationEditor : public RotationWidget {
      CheckeredSphereRotationEditor();
      
      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      virtual void redraw();
      //      virtual void resize(int width, int height);

      // -------------------------------------------------------
      // internal state
      // -------------------------------------------------------
    protected:
      vec3f color_bright, color_dark, color_bg;
    };
    
    // =======================================================
    //! a widget that allows for rotating a coordiante frame made up of three arrows (in R,G,B) 
    // =======================================================
    struct CoordFrameRotationEditor : public RotationWidget {
      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      virtual void redraw();
      //      virtual void resize(int width, int height);

      // -------------------------------------------------------
      // internal state
      // -------------------------------------------------------

    };


    // =======================================================
    //! render widget that renders through ospray
    // =======================================================
    struct OSPRayRenderWidget : public RotationWidget {
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
  }
}

