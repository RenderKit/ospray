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

#include "common/common.h"
#include "common/box.h"
#include "common/AffineSpace.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

#ifdef _WIN32
#  ifdef ospray_glut3d_EXPORTS
#    define OSPRAY_GLUT3D_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_GLUT3D_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPRAY_GLUT3D_INTERFACE
#endif


namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace glut3D {

    using namespace ospcommon;

    /*! initialize everything GLUT-related */
    OSPRAY_GLUT3D_INTERFACE void initGLUT(int32_t *ac, const char **av);
    /*! switch over to GLUT for control flow. This functoin will not return */
    OSPRAY_GLUT3D_INTERFACE void runGLUT();


    using ospcommon::AffineSpace3fa;

    /*! helper class that allows for easily computing (smoothed) frame rate */
    struct FPSCounter {
      OSPRAY_GLUT3D_INTERFACE FPSCounter();
      OSPRAY_GLUT3D_INTERFACE void startRender();
      OSPRAY_GLUT3D_INTERFACE void doneRender();
      OSPRAY_GLUT3D_INTERFACE double getFPS() const { return smooth_den / smooth_nom; }

    private:
      double smooth_nom;
      double smooth_den;
      double frameStartTime;
    };



    struct Glut3DWidget;
    
    struct Manipulator {
      // this is the fct that gets called when the mouse moved in the
      // associated window
      OSPRAY_GLUT3D_INTERFACE virtual void motion(Glut3DWidget *widget);
      // this is the fct that gets called when any mouse button got
      // pressed or released in the associated window
      OSPRAY_GLUT3D_INTERFACE virtual void button(Glut3DWidget *widget,
                                                  const vec2i &pos) {}
      /*! key press handler - override this fct to catch keyboard. */
      OSPRAY_GLUT3D_INTERFACE virtual void keypress(Glut3DWidget *widget,
                                                    const int32_t key);
      OSPRAY_GLUT3D_INTERFACE virtual void specialkey(Glut3DWidget *widget,
                                                      const int32_t key);
      OSPRAY_GLUT3D_INTERFACE Manipulator(Glut3DWidget *widget)
        : widget(widget) {}
    protected:

      // helper functions called from the default 'motion' fct
      OSPRAY_GLUT3D_INTERFACE virtual void dragLeft(Glut3DWidget *widget, 
                                                    const vec2i &to,
                                                    const vec2i &from)
      {}
      OSPRAY_GLUT3D_INTERFACE virtual void dragRight(Glut3DWidget *widget, 
                                                     const vec2i &to,
                                                     const vec2i &from)
      {}
      OSPRAY_GLUT3D_INTERFACE virtual void dragMiddle(Glut3DWidget *widget, 
                                                      const vec2i &to,
                                                      const vec2i &from)
      {}
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
      virtual void specialkey(Glut3DWidget *widget, const int32_t key);
      virtual void keypress(Glut3DWidget *widget, int32_t key);
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
      virtual void keypress(Glut3DWidget *widget, int32_t key);
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
      OSPRAY_GLUT3D_INTERFACE static vec2i defaultInitSize;

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
        float openingAngle; //!< in radians, along Y direction
        float aspect; //!< aspect ratio X:Y
        // float focalDistance;
      
        /*! camera frame in which the Y axis is the depth axis, and X
          and Z axes are parallel to the screen X and Y axis. The frame
          itself remains normalized. */
        AffineSpace3fa frame; 
      
        /*! set 'up' vector. if this vector is '0,0,0' the viewer will
         *not* apply the up-vector after camera manipulation */
        OSPRAY_GLUT3D_INTERFACE void snapUp();

        OSPRAY_GLUT3D_INTERFACE ViewPort();
      };
      // static InspectCenter INSPECT_CENTER;
      Manipulator *inspectCenterManipulator;
      Manipulator *moveModeManipulator;

      /*! current manipulator */
      Manipulator *manipulator;
      OSPRAY_GLUT3D_INTERFACE Glut3DWidget(FrameBufferMode frameBufferMode,
                                           ManipulatorMode initialManipulator=INSPECT_CENTER_MODE,
                                           int allowedManipulators=INSPECT_CENTER_MODE|MOVE_MODE);

      /*! set a default camera position that views given bounds from the
        top left front */
      OSPRAY_GLUT3D_INTERFACE virtual void setWorldBounds(const box3f &worldBounds);
      /*! tell GLUT that this window is 'dirty' and needs redrawing */
      OSPRAY_GLUT3D_INTERFACE virtual void forceRedraw();
      /*! set window title */
      OSPRAY_GLUT3D_INTERFACE void setTitle(const char *title);
      /*! set window title */
      OSPRAY_GLUT3D_INTERFACE void setTitle(const std::string &title) { setTitle(title.c_str()); }
      /*! set viewport to given values */
      OSPRAY_GLUT3D_INTERFACE void setViewPort(const vec3f from,
                                               const vec3f at,
                                               const vec3f up);

      // ------------------------------------------------------------------
      // event handling - override this to change this widgets behavior
      // to input events
      // ------------------------------------------------------------------
      OSPRAY_GLUT3D_INTERFACE virtual void mouseButton(int32_t which,
                                                       bool released,
                                                       const vec2i &pos);
      OSPRAY_GLUT3D_INTERFACE virtual void motion(const vec2i &pos);
      // /*! mouse moved to this location, with given left/right/middle buttons pressed */
      // virtual void mouseMotion(const vec2i &from, 
      //                          const vec2i &to,
      //                          bool left, 
      //                          bool right,
      //                          bool middle);
      OSPRAY_GLUT3D_INTERFACE virtual void reshape(const vec2i &newSize); 
      OSPRAY_GLUT3D_INTERFACE virtual void idle();
      /*! display this window. By default this will just clear this
        window's framebuffer; it's up to the user to override this fct
        to do something more useful */
      OSPRAY_GLUT3D_INTERFACE virtual void display();

      // ------------------------------------------------------------------
      // helper functions
      // ------------------------------------------------------------------
      /*! activate _this_ window, in the sense that all future glut
        events get routed to this window instance */
      OSPRAY_GLUT3D_INTERFACE virtual void activate();
      /*! create this window. Note that this just *creates* the window,
        but glut will not do anything else with this window before
        'run' got called */
      OSPRAY_GLUT3D_INTERFACE void create(const char *title,                               
                                          const vec2i &size=defaultInitSize,
                                          bool fullScreen = false);

      /*! clear the frame buffer color and depth bits */
      OSPRAY_GLUT3D_INTERFACE void clearPixels();

      /*! draw uint32_t pixels into the GLUT window (assumes window and buffer
       *  dimensions are equal) */
      OSPRAY_GLUT3D_INTERFACE void drawPixels(const uint32_t *framebuffer);

      /*! draw float4 pixels into the GLUT window (assumes window and buffer
       *  dimensions are equal) */
      OSPRAY_GLUT3D_INTERFACE void drawPixels(const vec3fa *framebuffer);

      // ------------------------------------------------------------------
      // camera helper code
      // ------------------------------------------------------------------
      OSPRAY_GLUT3D_INTERFACE void snapUp();
      /*! set 'up' vector. if this vector is '0,0,0' the viewer will
       *not* apply the up-vector after camera manipulation */
      OSPRAY_GLUT3D_INTERFACE virtual void setZUp(const vec3f &up);
      OSPRAY_GLUT3D_INTERFACE void noZUp() { setZUp(vec3f(0.f)); }

      // ------------------------------------------------------------------
      // internal state variables
      // ------------------------------------------------------------------
    public:
      vec2i lastMousePos; /*! last mouse screen position of mouse before
                            current motion */
      vec2i currMousePos; /*! current screen position of mouse */
      int64_t lastButtonState, currButtonState, currModifiers;
      ViewPort viewPort;
      box3f  worldBounds; /*!< world bounds, to automatically set viewPort
                            lookat, mouse speed, etc */
      int32_t windowID;
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
      OSPRAY_GLUT3D_INTERFACE void computeFrame();

      OSPRAY_GLUT3D_INTERFACE static Glut3DWidget *activeWindow;
      /*! pointer to the frame buffer data. it is the repsonsiblity of
          the applicatoin derived from this class to properly allocate
          and deallocate the frame buffer pointer */
      union {
        /*! uchar[4] RGBA-framebuffer, if applicable */
        uint32_t *ucharFB;
        /*! float[4] RGBA-framebuffer, if applicable */
        vec3fa *floatFB;
      };
      friend void glut3dReshape(int32_t x, int32_t y);
      friend void glut3dDisplay(void);
      friend void glut3dKeyboard(char key, int32_t x, int32_t y);
      friend void glut3dIdle(void);
      friend void glut3dMotionFunc(int32_t x, int32_t y);
      friend void glut3dMouseFunc(int32_t whichButton, int32_t released, 
                                  int32_t x, int32_t y);

      OSPRAY_GLUT3D_INTERFACE virtual void keypress(char key,
                                                    const vec2i &where);
      OSPRAY_GLUT3D_INTERFACE virtual void specialkey(int32_t key,
                                                      const vec2i &where);
    };

    std::ostream &operator<<(std::ostream &o, const Glut3DWidget::ViewPort &cam);
  }
}

