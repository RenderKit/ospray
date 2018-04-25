// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "ospcommon/common.h"
#include "ospcommon/box.h"
#include "ospcommon/AffineSpace.h"
#include "cameraManipulator.h"

#include "Imgui3dExport.h"

struct GLFWwindow;

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace imgui3D {

    using namespace ospcommon;

    /*! switch over to IMGUI for control flow. This func will not return */
    OSPRAY_IMGUI3D_INTERFACE void run();

    using ospcommon::AffineSpace3fa;

    /*! a IMGUI-based 3D viewer widget that includes simple sample code
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
    struct OSPRAY_IMGUI3D_INTERFACE ImGui3DWidget
    {
       typedef enum {
         FRAMEBUFFER_UCHAR,FRAMEBUFFER_FLOAT,FRAMEBUFFER_DEPTH,FRAMEBUFFER_NONE
       } FrameBufferMode;
       typedef enum {
         MOVE_MODE           =(1<<0),
         INSPECT_CENTER_MODE =(1<<1)
       } ManipulatorMode;

       /*! internal viewPort class */
       struct OSPRAY_IMGUI3D_INTERFACE ViewPort {
         bool modified; /* the viewPort will set this flag any time any of
                           its values get changed. */

         vec3f from;
         vec3f at;
         vec3f up;
         float openingAngle; //!< in degrees, along Y direction
         float aspect; //!< aspect ratio X:Y
         float apertureRadius;

         /*! camera frame in which the Y axis is the depth axis, and X
           and Z axes are parallel to the screen X and Y axis. The frame
           itself remains normalized. */
         AffineSpace3fa frame;

         /*! use current 'from', 'up', and 'at' to fix the 'up' vector according
             to */
         void snapViewUp();

         /*! set 'up' vector. if this vector is '0,0,0' the viewer will
          *not* apply the up-vector after camera manipulation */
         void snapFrameUp();

         ViewPort();
       };

       // static InspectCenter INSPECT_CENTER;
       std::unique_ptr<InspectCenter> inspectCenterManipulator;
       std::unique_ptr<MoveMode>      moveModeManipulator;

       /*! current manipulator */
       Manipulator *manipulator;

       ImGui3DWidget(FrameBufferMode frameBufferMode,
                     ManipulatorMode initialManipulator=INSPECT_CENTER_MODE);

       /*! set a default camera position that views given bounds from the
         top left front */
       virtual void setWorldBounds(const box3f &worldBounds);
       /*! set window title */
       void setTitle(const std::string &title);
       /*! set viewport to given values */
       void setViewPort(const vec3f from, const vec3f at, const vec3f up);

       // ------------------------------------------------------------------
       // event handling - override this to change this widgets behavior
       // to input events
       // ------------------------------------------------------------------

       void setMotionSpeed(float speed);

       virtual void motion(const vec2i &pos);
       virtual void mouseButton(int button, int action, int mods);
       virtual void reshape(const vec2i &newSize);
       /*! display this window. By default this will just clear this
           window's framebuffer; it's up to the user to override this fct
           to do something more useful */
       virtual void display();

       virtual void buildGui();

       // ------------------------------------------------------------------
       // helper functions
       // ------------------------------------------------------------------
       /*! create this window. Note that this just *creates* the window,
         but glut will not do anything else with this window before
         'run' got called */
       void create(const char *title, const bool fullScreen = false, vec2i windowSize = {1024, 768});

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
       vec2i lastMousePos; /*! last mouse screen position of mouse before
                             current motion */
       vec2i currMousePos; /*! current screen position of mouse */
       int lastButton[3], currButton[3];
       ViewPort viewPort;
       box3f  worldBounds; /*!< world bounds, to automatically set viewPort
                             lookat, mouse speed, etc */
       vec2i windowSize;
       /*! camera speed modifier - affects how many units the camera
          _moves_ with each unit on the screen */
       float motionSpeed {-1.f};
       /*! camera rotation speed modifier - affects how many units the
          camera _rotates_ with each unit on the screen */
       float rotateSpeed;
       FrameBufferMode frameBufferMode;

       /*! recompute current viewPort's frame from cameras 'from',
           'at', 'up' values. */
       void computeFrame();

       static ImGui3DWidget *activeWindow;

       static bool animating;
       static bool showGui;
       double displayTime;
       double renderTime;
       double guiTime;
       double totalTime;
       float  fontScale;
       bool upAnchored {true};

       bool renderingPaused {false};
       /*! pointer to the frame buffer data. it is the repsonsiblity of
           the applicatoin derived from this class to properly allocate
           and deallocate the frame buffer pointer */
       union {
         /*! uchar[4] RGBA-framebuffer, if applicable */
         uint32_t *ucharFB;
         /*! float[4] RGBA-framebuffer, if applicable */
         vec3fa *floatFB;
       };

       GLFWwindow *window {nullptr};

       virtual void keypress(char key);
    };

    OSPRAY_IMGUI3D_INTERFACE std::ostream &operator<<(std::ostream &o,
                                            const ImGui3DWidget::ViewPort &cam);
  }
}

