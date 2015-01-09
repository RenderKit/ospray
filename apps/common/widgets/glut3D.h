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

#pragma once

#include "ospray/common/OSPCommon.h"
#include /*embree*/"common/math/affinespace.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace glut3D {

    /*! initialize everything GLUT-related */
    void initGLUT(int32 *ac, const char **av);
    /*! switch over to GLUT for control flow. This functoin will not return */
    void runGLUT();


    using embree::AffineSpace3fa;

    /*! helper class that allows for easily computing (smoothed) frame rate */
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



    struct Glut3DWidget;
    
    struct Manipulator {
      // this is the fct that gets called when the mouse moved in the
      // associated window
      virtual void motion(Glut3DWidget *widget);
      // this is the fct that gets called when any mouse button got
      // pressed or released in the associated window
      virtual void button(Glut3DWidget *widget, const vec2i &pos) {};
      /*! key press handler - override this fct to catch keyboard. */
      virtual void keypress(Glut3DWidget *widget, const int32 key);
      virtual void specialkey(Glut3DWidget *widget, const int32 key);
      Manipulator(Glut3DWidget *widget) : widget(widget) {};
    protected:

      // helper functions called from the default 'motion' fct
      virtual void dragLeft(Glut3DWidget *widget, 
                            const vec2i &to, const vec2i &from) 
      {};
      virtual void dragRight(Glut3DWidget *widget, 
                             const vec2i &to, const vec2i &from) 
      {};
      virtual void dragMiddle(Glut3DWidget *widget, 
                              const vec2i &to, const vec2i &from)
      {};
      Glut3DWidget *widget;
    };

    struct InspectCenter : public Manipulator
    {
      virtual void dragLeft(Glut3DWidget *widget, 
                            const vec2i &to, const vec2i &from);
      virtual void dragRight(Glut3DWidget *widget, 
                             const vec2i &to, const vec2i &from);
      virtual void dragMiddle(Glut3DWidget *widget, 
                              const vec2i &to, const vec2i &from);
      virtual void specialkey(Glut3DWidget *widget, const int32 key);
      virtual void keypress(Glut3DWidget *widget, int32 key);
      virtual void button(Glut3DWidget *widget, const vec2i &pos);
      InspectCenter(Glut3DWidget *widget);
      void rotate(float du, float dv);

      vec3f pivot;
    };

    struct MoveMode : public Manipulator
    {
      virtual void dragLeft(Glut3DWidget *widget, 
                            const vec2i &to, const vec2i &from);
      virtual void dragRight(Glut3DWidget *widget, 
                             const vec2i &to, const vec2i &from);
      virtual void dragMiddle(Glut3DWidget *widget, 
                              const vec2i &to, const vec2i &from);
      virtual void keypress(Glut3DWidget *widget, int32 key);
      virtual void button(Glut3DWidget *widget, const vec2i &pos) {}
      MoveMode(Glut3DWidget *widget) : Manipulator(widget) {}
    };




    /*! a GLUT-based 3D viewer widget that includes simple sample code
      for manipulating a 3D viewPort with the mouse.

      This widget should allow users to easily write simple 3D viewers
      with simple built-in viewPort motion. In theory, all one hsa to do
      after creating the window (and initializeing the viewPort is
      implement the respective 'display' callback to do the actual
      rendering, using the widget's infrastructure for the boilerplace
      stuff.

      If specified (\see FrameBufferMode the widget will automatically
      allocate a frame buffer (of either uchar4 or float4 type), in
      which case the 'display' routine will only have to write to the
      respective frame buffer (the widget will do the respective gl
      calls to display that framebuffer); if not specified, it's up to
      the derived class to implement whatever opengl calls are required
      to draw the window's content.
    
    */
    struct Glut3DWidget
    {
      /*! size we'll create a window at */
      static vec2i defaultInitSize;

      typedef enum { 
        FRAMEBUFFER_UCHAR,FRAMEBUFFER_FLOAT,FRAMEBUFFER_DEPTH,FRAMEBUFFER_NONE
      } FrameBufferMode;
      typedef enum {
        MOVE_MODE           =(1<<0),
        INSPECT_CENTER_MODE =(1<<1)
      } ManipulatorMode;
      /*! internal viewPort class */
      struct ViewPort {
        bool modified; /* the viewPort will set this flag any time any of
                          its values get changed. */

        vec3f from;
        vec3f up;
        vec3f at;
        /*! opening angle, in radians, along Y direction */
        float openingAngle;
        /*! aspect ration i Y:X */
        float aspect;
        // float focalDistance;
      
        /*! camera frame in which the Y axis is the depth axis, and X
          and Z axes are parallel to the screen X and Y axis. The frame
          itself remains normalized. */
        AffineSpace3fa frame; 
      
        /*! set 'up' vector. if this vector is '0,0,0' the viewer will
         *not* apply the up-vector after camera manipulation */
        void snapUp();

        ViewPort();
      };
      // static InspectCenter INSPECT_CENTER;
      Manipulator *inspectCenterManipulator;
      Manipulator *moveModeManipulator;

      /*! current manipulator */
      Manipulator *manipulator;
      Glut3DWidget(FrameBufferMode frameBufferMode,
                   ManipulatorMode initialManipulator=INSPECT_CENTER_MODE,
                   int allowedManipulators=INSPECT_CENTER_MODE|MOVE_MODE);

      /*! set a default camera position that views given bounds from the
        top left front */
      virtual void setWorldBounds(const box3f &worldBounds);
      /*! tell GLUT that this window is 'dirty' and needs redrawing */
      virtual void forceRedraw();
      /*! set window title */
      void setTitle(const char *title);
      /*! set window title */
      void setTitle(const std::string &title) { setTitle(title.c_str()); }
      /*! set viewport to given values */
      void setViewPort(const vec3f from, const vec3f at, const vec3f up);

      // ------------------------------------------------------------------
      // event handling - override this to change this widgets behavior
      // to input events
      // ------------------------------------------------------------------
      virtual void mouseButton(int32 which, bool released, const vec2i &pos);
      virtual void motion(const vec2i &pos);
      // /*! mouse moved to this location, with given left/right/middle buttons pressed */
      // virtual void mouseMotion(const vec2i &from, 
      //                          const vec2i &to,
      //                          bool left, 
      //                          bool right,
      //                          bool middle);
      virtual void reshape(const vec2i &newSize); 
      virtual void idle();
      /*! display this window. By default this will just clear this
        window's framebuffer; it's up to the user to override this fct
        to do something more useful */
      virtual void display();

      // ------------------------------------------------------------------
      // helper functions
      // ------------------------------------------------------------------
      /*! activate _this_ window, in the sense that all future glut
        events get routed to this window instance */
      virtual void activate();
      /*! create this window. Note that this just *creates* the window,
        but glut will not do anything else with this window before
        'run' got called */
      void create(const char *title,                               
                  const vec2i &size=defaultInitSize,
                  bool fullScreen = false);

      /*! clear the frame buffer color and depth bits */
      void clearPixels();

      /*! draw uint32 pixels into the GLUT window (assumes window and buffer dimensions are equal) */
      void drawPixels(const uint32 *framebuffer);

      /*! draw float4 pixels into the GLUT window (assumes window and buffer dimensions are equal) */
      void drawPixels(const vec3fa *framebuffer);

      // ------------------------------------------------------------------
      // camera helper code
      // ------------------------------------------------------------------
      void snapUp();
      /*! set 'up' vector. if this vector is '0,0,0' the viewer will
       *not* apply the up-vector after camera manipulation */
      virtual void setZUp(const vec3f &up);
      void noZUp() { setZUp(vec3f(0.f)); }

      // ------------------------------------------------------------------
      // internal state variables
      // ------------------------------------------------------------------
    public:
      vec2i lastMousePos; /*! last mouse screen position of mouse before
                            current motion */
      vec2i currMousePos; /*! current screen position of mouse */
      int64 lastButtonState, currButtonState, currModifiers;
      ViewPort viewPort;
      box3f  worldBounds; /*!< world bounds, to automatically set viewPort
                            lookat, mouse speed, etc */
      int32 windowID;
      vec2i windowSize;
      /*! camera speed modifier - affects how many units the camera
         _moves_ with each unit on the screen */
      float motionSpeed;
      /*! camera rotation speed modifier - affects how many units the
         camera _rotates_ with each unit on the screen */
      float rotateSpeed;
      FrameBufferMode frameBufferMode;

      /*! recompute current viewPort's frame from cameras 'from',
          'at', 'up' values. */
      void computeFrame();

      static Glut3DWidget *activeWindow;
      union {
        /*! uchar[4] RGBA-framebuffer, if applicable */
        uint32 *ucharFB;
        /*! float[4] RGBA-framebuffer, if applicable */
        vec3fa *floatFB;
        /*! floating point depth framebuffer, if applicable */
        float *depthFB;
      };
      friend void glut3dReshape(int32 x, int32 y);
      friend void glut3dDisplay(void);
      friend void glut3dKeyboard(char key, int32 x, int32 y);
      friend void glut3dIdle(void);
      friend void glut3dMotionFunc(int32 x, int32 y);
      friend void glut3dMouseFunc(int32 whichButton, int32 released, 
                                  int32 x, int32 y);

      virtual void keypress(char key, const vec2f where);
      virtual void specialkey(int32 key, const vec2f where);
    };

    std::ostream &operator<<(std::ostream &o, const Glut3DWidget::ViewPort &cam);
  }
}

