#include "glut3D.h"
#ifdef __APPLE__
#include "GLUT/glut.h"
#else
#include "GL/glut.h"
#endif

namespace ospray {

  namespace glut3D {

    /*! currently active window */
    Glut3DWidget *Glut3DWidget::activeWindow = NULL;
    // InspectCenter Glut3DWidget::INSPECT_CENTER;


    // ------------------------------------------------------------------
    // glut event handlers
    // ------------------------------------------------------------------

    void glut3dReshape( int x, int y )
    {
      if (Glut3DWidget::activeWindow) 
        Glut3DWidget::activeWindow->reshape(vec2i(x,y));
    }

    void glut3dDisplay( void )
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->display();
    }

    void glut3dKeyboard(unsigned char key, int x, int y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->keypress(key,vec2i(x,y));
    }

    void glut3dIdle( void )
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->idle();
    }
    void glut3dMotionFunc(int x, int y)
    {
      if (Glut3DWidget::activeWindow) 
        Glut3DWidget::activeWindow->motion(vec2i(x,y));
    }

    void glut3dMouseFunc(int whichButton, int released, int x, int y)
    {
      if (Glut3DWidget::activeWindow) 
        Glut3DWidget::activeWindow->mouseButton(whichButton,released,vec2i(x,y));
    }
  

    // ------------------------------------------------------------------
    // implementation of glut3d::viewPorts
    // ------------------------------------------------------------------
    Glut3DWidget::ViewPort::ViewPort()
      : from(0,0,-1),
        at(0,0,0),
        up(0,1,0),
      // : from(0,-1,0),
      //   at(0,0,0),
      //   up(0,0,1),
        aspectRatio(1.f),
        openingAngle(60.f*M_PI/360.f),
        modified(true)
    {
      frame = AffineSpace3f::translate(from) * AffineSpace3f(embree::one);
    }

    void Glut3DWidget::ViewPort::snapUp()
    {
      if (fabsf(dot(up,frame.l.vz)) < 1e-3f)
        return;
      frame.l.vx = normalize(cross(frame.l.vy,up));
      frame.l.vz = normalize(cross(frame.l.vx,frame.l.vy));
      frame.l.vy = normalize(cross(frame.l.vz,frame.l.vx));
    }

    // ------------------------------------------------------------------
    // implementation of glut3d widget
    // ------------------------------------------------------------------
    void Glut3DWidget::mouseButton(int whichButton, bool released, const vec2i &pos)
    {
    
      if (pos != currMousePos)
        motion(pos);
      lastButtonState = currButtonState;
      if (released)
        currButtonState = currButtonState & ~(1<<whichButton);
      else
        currButtonState = currButtonState |  (1<<whichButton);
      manipulator->button(this);
    }
    void Glut3DWidget::motion(const vec2i &pos)
    {
      currMousePos = pos;
      if (currButtonState != lastButtonState) {
        // some button got pressed; reset 'old' pos to new pos.
        lastMousePos = currMousePos;
        lastButtonState = currButtonState;
      }
    
      // bool left   = currButtonState & (1<<GLUT_LEFT_BUTTON);
      // bool right  = currButtonState & (1<<GLUT_RIGHT_BUTTON);
      // bool middle = currButtonState & (1<<GLUT_MIDDLE_BUTTON);

      //mouseMotion(currMousePos,lastMousePos,left,right,middle);
      manipulator->motion(this);
      lastMousePos = currMousePos;
      if (viewPort.modified)
        forceRedraw();
    }
  
    Glut3DWidget::Glut3DWidget(FrameBufferMode frameBufferMode, Manipulator *manipulator)
      : motionSpeed(.003f),
        windowID(-1),
        lastMousePos(-1,-1),
        currMousePos(-1,-1),
        windowSize(-1,-1),
        lastModifierState(0), 
        currModifierState(0),
        lastButtonState(0), 
        currButtonState(0),
        frameBufferMode(frameBufferMode),
        manipulator(manipulator),
        ucharFB(NULL)
    {
      worldBounds.lower = vec3f(-1);
      worldBounds.upper = vec3f(+1);
    }

    void Glut3DWidget::setZUp(const vec3f &up)
    {
      viewPort.up = up;
      if (up != vec3f(0.f)) {
        viewPort.snapUp();
        forceRedraw();
      }
    }

    void Glut3DWidget::reshape(const vec2i &newSize) 
    { 
      windowSize = newSize; 
      viewPort.aspectRatio = newSize.x/float(newSize.y);

      switch (frameBufferMode) {
      case FRAMEBUFFER_UCHAR:
        if (ucharFB) delete[] ucharFB;
        ucharFB = new unsigned int[newSize.x*newSize.y];
        break;
      case FRAMEBUFFER_FLOAT:
        if (floatFB) delete[] floatFB;
        floatFB = new vec3fa[newSize.x*newSize.y];
        break;
      default:
        break;
      }

      forceRedraw();
    }
    void Glut3DWidget::activate()
    {
      activeWindow = this;
      glutSetWindow(windowID);
    }


    void Glut3DWidget::forceRedraw()
    {
      glutPostRedisplay();
    }

    void Glut3DWidget::display()
    {
      if (frameBufferMode == Glut3DWidget::FRAMEBUFFER_UCHAR && ucharFB) {
        glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, ucharFB);
      } else if (frameBufferMode == Glut3DWidget::FRAMEBUFFER_FLOAT && floatFB) {
        glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_FLOAT, floatFB);
      } else {
        glClearColor(0.f,0.f,0.f,1.f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      }
      glutSwapBuffers();
    }

    void Glut3DWidget::setWorldBounds(const box3f &worldBounds)
    {
      vec3f center = embree::center(worldBounds);
      vec3f diag   = worldBounds.size();
      diag         = max(diag,vec3f(0.3f*length(diag)));
      vec3f from   = center - .75f*vec3f(-.6*diag.x,-1.2*diag.y,.8*diag.z);
      vec3f dir    = center - from;
      vec3f up     = viewPort.up;

      viewPort.at    = center;
      viewPort.from  = from;
      viewPort.up    = up;

      if (length(up) < 1e-3f)
        up = vec3f(0,0,1.f);

      this->worldBounds = worldBounds;
      viewPort.frame.l.vy = normalize(dir);
      viewPort.frame.l.vx = normalize(cross(viewPort.frame.l.vy,up));
      viewPort.frame.l.vz = normalize(cross(viewPort.frame.l.vx,viewPort.frame.l.vy));
      viewPort.frame.p    = from;
      viewPort.snapUp();
      viewPort.modified = true;
    }

    void Glut3DWidget::setTitle(const char *title) 
    {
      glutSetWindowTitle(title); 
    }
  
    void Glut3DWidget::create(const char *title, bool fullScreen)
    {
      glutInitWindowSize( 1024, 768 );
      windowID = glutCreateWindow(title);
      activeWindow = this;
      glutDisplayFunc( glut3dDisplay );
      glutReshapeFunc( glut3dReshape );  
      glutKeyboardFunc(glut3dKeyboard );
      glutMotionFunc(  glut3dMotionFunc );
      glutMouseFunc(   glut3dMouseFunc );
      glutIdleFunc(    glut3dIdle );

      if (fullScreen)
        glutFullScreen();
    }

    void runGLUT()
    {
      glutMainLoop();
    }

    void initGLUT(int *ac, const char **av)
    {
      glutInit(ac,(char **)av);
    }

    // ------------------------------------------------------------------
    // base manipulator
    // ------------------------------------------------------------------
    void Manipulator::motion(Glut3DWidget *widget)
    {
      if (widget->currButtonState == (1<<GLUT_LEFT_BUTTON)) {
        dragLeft(widget,widget->currMousePos,widget->lastMousePos);
      } else if (widget->currButtonState == (1<<GLUT_RIGHT_BUTTON)) {
        dragRight(widget,widget->currMousePos,widget->lastMousePos);
      } else if (widget->currButtonState == (1<<GLUT_MIDDLE_BUTTON)) {
        dragMiddle(widget,widget->currMousePos,widget->lastMousePos);
      }
    }

    // ------------------------------------------------------------------
    // INSPECT_CENTER manipulator
    // ------------------------------------------------------------------

    /*! INSPECT_CENTER::RightButton: move lookfrom/viewPort positoin
      forward/backward on right mouse button */
    void InspectCenter::dragRight(Glut3DWidget *widget, 
                                  const vec2i &to, const vec2i &from) 
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float fwd = (to.y - from.y) * 4 * widget->motionSpeed
        * length(widget->worldBounds.size());
      float oldDist = length(cam.at - cam.from);
      float newDist = oldDist - fwd;
      if (newDist < 1e-3f) 
        return;
      cam.from = cam.at - newDist * cam.frame.l.vy;
      cam.frame.p = cam.from;
      cam.modified = true;
    }

    /*! INSPECT_CENTER::MiddleButton: move lookat/center of interest
      forward/backward on middle mouse button */
    void InspectCenter::dragMiddle(Glut3DWidget *widget, 
                                   const vec2i &to, const vec2i &from) 
    {
      // it's called inspect_***CENTER*** for a reason; this class
      // will keep the rotation pivot at the center, and not do
      // anything with center mouse button...
    }

    void InspectCenter::dragLeft(Glut3DWidget *widget, 
                                 const vec2i &to, const vec2i &from) 
    {
      std::cout << "-------------------------------------------------------" << std::endl;
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->motionSpeed;
      float dv = (to.y - from.y) * widget->motionSpeed;

      vec2i delta_mouse = to - from;
      PRINT(delta_mouse);

      const vec3f pivot = center(widget->worldBounds);
      AffineSpace3f xfm 
        = AffineSpace3f::translate(pivot)
        * AffineSpace3f::rotate(cam.frame.l.vx,-dv)
        * AffineSpace3f::rotate(cam.frame.l.vz,-du)
        * AffineSpace3f::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapUp();
      cam.modified = true;
    }


    void Manipulator::keypress(Glut3DWidget *widget, const int key) 
    {
      if (key == 'Q') {
        exit(0);
      }
    };


    std::ostream &operator<<(std::ostream &o, const Glut3DWidget::ViewPort &cam)
    {
      o << "<viewPort>" << std::endl;
      o << "  <from>" << cam.from.x << " " << cam.from.y << " " << cam.from.z << "</from>" << std::endl;
      o << "  <at>" << cam.at.x << " " << cam.at.y << " " << cam.at.z << "</at>" << std::endl;
      o << "  <up>" << cam.up.x << " " << cam.up.y << " " << cam.up.z << "</up>" << std::endl;
      o << "  <aspect>" << cam.aspectRatio << "</aspect>" << std::endl;
      o << "  <frame.dx>" << cam.frame.l.vx << "</frame.dx>" << std::endl;
      o << "  <frame.dy>" << cam.frame.l.vy << "</frame.dy>" << std::endl;
      o << "  <frame.dz>" << cam.frame.l.vz << "</frame.dz>" << std::endl;
      o << "  <frame.p>" << cam.frame.p << "</frame.p>" << std::endl;
      o << "</viewPort>";
      return o;
    }
      // struct ViewPort {
      //   bool modified; /* the viewPort will set this flag any time any of
      //                     its values get changed. */

      //   vec3f from;
      //   vec3f up;
      //   vec3f at;
      //   /*! opening angle, in radians, along Y direction */
      //   float openingAngle;
      //   /*! aspect ration i Y:X */
      //   float aspectRatio;
      //   // float focalDistance;
      
      //   /*! viewPort frame in which the Y axis is the depth axis, and X
      //     and Z axes are parallel to the screen X and Y axis. The frame
      //     itself remains normalized. */
      //   AffineSpace3f frame; 
      
      //   /*! set 'up' vector. if this vector is '0,0,0' the viewer will
      //    *not* apply the up-vector after viewPort manipulation */
      //   void snapUp();



  }
}

