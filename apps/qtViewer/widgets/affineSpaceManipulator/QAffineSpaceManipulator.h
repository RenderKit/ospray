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

#pragma once

// sg
#include "sg/SceneGraph.h"
// ospray public api
#include "ospray/ospray.h"
// ospcomon
#include "ospcommon/box.h"
#include "ospcommon/LinearSpace.h"
// qt
#include <QtGui>
#include <QGLWidget>

namespace ospray {
  namespace viewer {
    using namespace ospcommon;
    typedef LinearSpace3f linear3f;

    // ==================================================================
    //! \brief A QT Widget that allows mouse manipulation of a reference
    /*! The Widget works by keeping and manipulating a reference frame
      defined by two points (source and target), and a linear
      space. THe y axis if the linear space always points from
      source point to target point, thereby defining a natural
      "towards" direction (such as camera pointing towards lookat
      point, or spotlight pointing towards a point it's
      illuminating), while source and target define some natural
      pair of points associated with this direction (like said
      camera origin and camera lookat, or spotlight position and
      spotlight target position). By defining obvious directionality
      (forward=='from source to target') and orientation ('frame.vy
      points 'upwards'') this also allows for obvious manipulations
      such as "move X units forward", or "move Y units to the left",
      "snap frame to point upwards", etc, which this widget maps to mouse motion.

      How some given mouse motion affects the reference space is
      goverennd by the 'interactionMode' that can be set to
      different modes such as a 'fly' mode (representing the way a
      3D shooter game might allow a player to navigate in 3D space),
      or a 'inspect' mode (rotating the camera around some point of
      interest, etc.        
    */
    // ==================================================================
    class QAffineSpaceManipulator : public QGLWidget {
      Q_OBJECT;

    public:
      //! camera manipulation mode we're in (determines how mouse motion influences the camera pose)
      typedef enum { 
        //! mode in which the camera emulates a person 'flying' through the scene
        FLY, 
          
        //! mode in which the camera rotates ardound the center of
        //! interest (the lookat). Free rotation will *NOT* apply
        //! the lookup vector - ever.
        FREE_ROTATION,

        //! mode in which the camera rotates ardound the center of
        //! interest (the lookat), but where the user can also
        //! change the center of inspection
        INSPECT,
      } InteractionMode;

      //! the reference frame we're manipulating
      struct ReferenceFrame {
        //! constructor
        ReferenceFrame();

        //! the position of the point of interest we're rotating around the pivot point
        vec3f sourcePoint;

        //! the pivot point we're rotating around (e.g., the 'lookat' point for a camera rotation widget)
        vec3f targetPoint;

        //! the normalized coordinate frame, with 'vy' pointing from
        //! 'source' to 'target', and 'vz' pointing 'upward' (and 'vx'
        //! being right-handed orthogonal to y and z)
        linear3f orientation;

        //! look-up vector; can be '(0,0,0)' to indicate that no
        //! lookup'ing will be done.  if this vector is non-null, we
        //! will always snap frame.vz to be co-planar with the plane
        //! spanned by 'frame.vy' and 'up'
        vec3f upVector; 

        //! snap the orientation back to be aligned with the upvector. if upvector is (0,0,0), this fct will do nothing
        void snapUp();
      };

    signals:
      //! the signal we're emitting to tell our listener(s) that something has changed
      void affineSpaceChanged(QAffineSpaceManipulator *which);
      
    public:
      //! constructor
      QAffineSpaceManipulator(QAffineSpaceManipulator::InteractionMode interactionMode = FLY);
      
      //! switch to requested interaction mode
      void setInteractionMode(QAffineSpaceManipulator::InteractionMode interactionMode);

      /*! toggle-up: switch to given axis (x=0,y=1,z=2) as up-vectors for the rotation.
        when _already_ in up-vector mode for this axis, switch to negative axis. */
      void toggleUp(int axis);

      //! set the movement speed
      void setMoveSpeed(const float moveSpeed) { this->motionSpeed = moveSpeed; }
      
      //! set camera to a default pose that overlooks the given bounding box
      void setDefaultFrame(const box3f &worldBounds);

      //! return the camera
      const ReferenceFrame *getFrame() const { return frame; }
      
      //! return size of widget in pixels
      inline vec2i getSize() const { return size; }

      /*! rotate around target point, by given angles */
      void rotateAroundTarget(float angle_x, float angle_y);

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
      // overloadable callbacks
      // -------------------------------------------------------

      //! the method our 'paintGL' callback calls to tell the widget to paint its geometry
      virtual void redraw() {};

      //! this method gets called whenever the wiget gets resized
      virtual void resize(int width, int height) {};

      //! this fct gets called when the user triggers a 'pick' event (by clicking with the pick modifier key pressed)
      virtual void pick(QMouseEvent *event)
      { std::cout << "pcking at " << event->pos().x() << "." << event->pos().y() << std::endl; }

    protected:

      virtual void strafe(QMouseEvent * event);
      virtual void rotate(QMouseEvent * event);      
      virtual void move(QMouseEvent * event);

      //! size of window, in pixels
      vec2i         size; 
      //! viewport the user is modifying
      ReferenceFrame *frame;
      //! camera manipulation mode we're in (determines how mouse motion influences the camera pose)
      InteractionMode interactionMode;
      //! speed at which the frame gets *moved*. If set to 0, the frame will still rotate, but motion will be disabled.
      float motionSpeed;

      QPoint lastMousePos;
      //! this stores the orientation at the *START* of a dragging motion
      linear3f orientationWhenMouseGotPressed;
    };

    // =======================================================
    //! a widget that allows for rotating a checkered sphere
    // =======================================================
    struct QCheckeredBallFrameEditor : public QAffineSpaceManipulator {
      QCheckeredBallFrameEditor();
      
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
    struct QCoordAxisFrameEditor : public QAffineSpaceManipulator {
      QCoordAxisFrameEditor(QAffineSpaceManipulator::InteractionMode interactionMode = FLY)
        : QAffineSpaceManipulator(interactionMode) 
      {}

      // -------------------------------------------------------
      // render widget callbacks
      // -------------------------------------------------------

      virtual void redraw();

      // -------------------------------------------------------
      // internal state
      // -------------------------------------------------------
    };


  }
}

