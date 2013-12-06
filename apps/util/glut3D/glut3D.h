#include "ospray/common/ospcommon.h"
#include /*embree*/"common/math/affinespace.h"

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace glut3D {

    /*! initialize everything GLUT-related */
    void initGLUT(int *ac, const char **av);
    /*! switch over to GLUT for control flow. This functoin will not return */
    void runGLUT();


    using embree::AffineSpace3f;

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

    /*! a GLUT-based 3D viewer widget that includes simple sample code
      for manipulating a 3D camera with the mouse.

      This widget should allow users to easily write simple 3D viewers
      with simple built-in camera motion. In theory, all one hsa to do
      after creating the window (and initializeing the camera is
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
      typedef enum { 
        FRAMEBUFFER_UCHAR,FRAMEBUFFER_FLOAT,FRAMEBUFFER_NONE
      } FrameBufferMode;
      /*! internal camera class */
      struct Camera {
        bool modified; /* the camera will set this flag any time any of
                          its values get changed. */

        vec3f from;
        vec3f up;
        vec3f at;
        /*! opening angle, in radians, along Y direction */
        float openingAngle;
        /*! aspect ration i Y:X */
        float aspectRatio;
        // float focalDistance;
      
        /*! camera frame in which the Y axis is the depth axis, and X
          and Z axes are parallel to the screen X and Y axis. The frame
          itself remains normalized. */
        AffineSpace3f frame; 
      
        /*! set 'up' vector. if this vector is '0,0,0' the viewer will
         *not* apply the up-vector after camera manipulation */
        void snapUp();

        Camera();
      };

      struct Manipulator {
        // this is the fct that gets called when the mouse moved in the
        // associated window
        virtual void motion(Glut3DWidget *widget);
        // this is the fct that gets called when any mouse button got
        // pressed or released in the associated window
        virtual void button(Glut3DWidget *widget) {};
        /*! key press handler - override this fct to catch keyboard */
        virtual void keypress(Glut3DWidget *widget, const int key);
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
      };
      struct InspectCenter : public Manipulator
      {
        virtual void dragLeft(Glut3DWidget *widget, 
                              const vec2i &to, const vec2i &from);
        virtual void dragRight(Glut3DWidget *widget, 
                               const vec2i &to, const vec2i &from);
        virtual void dragMiddle(Glut3DWidget *widget, 
                                const vec2i &to, const vec2i &from);
      };
      static InspectCenter INSPECT_CENTER;

      Glut3DWidget(FrameBufferMode frameBufferMode, Manipulator *manipulator);

      /*! set a default camera position that views given bounds from the
        top left front */
      virtual void setWorldBounds(const box3f &worldBounds);
      /*! tell GLUT that this window is 'dirty' and needs redrawing */
      virtual void forceRedraw();
      /*! set window title */
      void setTitle(const char *title);
      /*! set window title */
      void setTitle(const std::string &title) { setTitle(title.c_str()); }

      // ------------------------------------------------------------------
      // event handling - override this to change this widgets behavior
      // to input events
      // ------------------------------------------------------------------
      virtual void mouseButton(int which, bool released, const vec2i &pos);
      virtual void motion(const vec2i &pos);
      // /*! mouse moved to this location, with given left/right/middle buttons pressed */
      // virtual void mouseMotion(const vec2i &from, 
      //                          const vec2i &to,
      //                          bool left, 
      //                          bool right,
      //                          bool middle);
      virtual void reshape(const vec2i &newSize); 
      virtual void idle()                        {}
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
      void create(const char *title, bool fullScreen = false);

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
    protected:
      vec2i lastMousePos; /*! last mouse screen position of mouse before
                            current motion */
      vec2i currMousePos; /*! current screen position of mouse */
      int lastModifierState, currModifierState;
      int lastButtonState, currButtonState;
      Camera camera;
      box3f  worldBounds; /*!< world bounds, to automatically set camera
                            lookat, mouse speed, etc */
      Manipulator *manipulator;
      int windowID;
      vec2i windowSize;
      /* camera speed modifier */
      float cameraSpeed;
      FrameBufferMode frameBufferMode;

      static Glut3DWidget *activeWindow;
      union {
        /*! uchar[4] RGBA-framebuffer, if applicable */
        unsigned int *ucharFB;
        /*! float[4] RGBA-framebuffer, if applicable */
        vec3fa       *floatFB;
      };
      friend void glut3dReshape( int x, int y );
      friend void glut3dDisplay( void );
      friend void glut3dKeyboard(unsigned char key, int x, int y);
      friend void glut3dIdle( void );
      friend void glut3dMotionFunc(int x, int y);
      friend void glut3dMouseFunc(int whichButton, int released, int x, int y);

      virtual void keypress(unsigned char key, const vec2f where)
      { if (manipulator) manipulator->keypress(this,key); }
    };
  }
}

