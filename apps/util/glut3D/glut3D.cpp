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

    int g_width = 1024, g_height = 768;

    void error(const std::string &msg)
    {
      std::cout << "ospray::glut3D fatal error : " << msg << std::endl;
      std::cout << std::endl;
      std::cout << "Proper usage: " << std::endl;
      std::cout << " -win <width>x<height>" << std::endl;
      std::cout << std::endl;
      exit(1);
    }
    // InspectCenter Glut3DWidget::INSPECT_CENTER;
    /*! viewport as specified on the command line */
    Glut3DWidget::ViewPort *viewPortFromCmdLine = NULL;


    // ------------------------------------------------------------------
    // glut event handlers
    // ------------------------------------------------------------------

    void glut3dReshape(int32 x, int32 y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->reshape(vec2i(x,y));
    }

    void glut3dDisplay( void )
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->display();
    }

    void glut3dKeyboard(unsigned char key, int32 x, int32 y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->keypress(key,vec2i(x,y));
    }
    void glut3dSpecial(int32 key, int32 x, int32 y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->specialkey(key,vec2i(x,y));
    }

    void glut3dIdle( void )
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->idle();
    }
    void glut3dMotionFunc(int32 x, int32 y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->motion(vec2i(x,y));
    }

    void glut3dMouseFunc(int32 whichButton, int32 released, int32 x, int32 y)
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
    aspect(1.f),
    openingAngle(60.f*M_PI/360.f),
    modified(true)
    {
      frame = AffineSpace3fa::translate(from) * AffineSpace3fa(embree::one);
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
    void Glut3DWidget::mouseButton(int32 whichButton, bool released, const vec2i &pos)
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

    Glut3DWidget::Glut3DWidget(FrameBufferMode frameBufferMode,
     ManipulatorMode initialManipulator,
     int allowedManipulators)
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
    ucharFB(NULL)
    {
      worldBounds.lower = vec3f(-1);
      worldBounds.upper = vec3f(+1);

      if (allowedManipulators & INSPECT_CENTER_MODE) {
        inspectCenterManipulator = new InspectCenter(this);
      }
      if (allowedManipulators & MOVE_MODE) {
        moveModeManipulator = new MoveMode(this);
      }
      switch(initialManipulator) {
        case MOVE_MODE:
        manipulator = moveModeManipulator;
        break;
        case INSPECT_CENTER_MODE:
        manipulator = inspectCenterManipulator;
        break;
      }
      Assert2(manipulator != NULL,"invalid initial manipulator mode");

      if (viewPortFromCmdLine) {
        viewPort = *viewPortFromCmdLine;

        if (length(viewPort.up) < 1e-3f)
          viewPort.up = vec3f(0,0,1.f);

        this->worldBounds = worldBounds;
        viewPort.frame.l.vy = normalize(viewPort.at - viewPort.from);
        viewPort.frame.l.vx = normalize(cross(viewPort.frame.l.vy,viewPort.up));
        viewPort.frame.l.vz = normalize(cross(viewPort.frame.l.vx,viewPort.frame.l.vy));
        viewPort.frame.p    = viewPort.from;
        viewPort.snapUp();
        viewPort.modified = true;
      }
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
      viewPort.aspect = newSize.x/float(newSize.y);

      switch (frameBufferMode) {
        case FRAMEBUFFER_UCHAR:
        if (ucharFB) delete[] ucharFB;
        ucharFB = new uint32[newSize.x*newSize.y];
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

      if (!viewPortFromCmdLine) {
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
    }

    void Glut3DWidget::setTitle(const char *title)
    {
      Assert2(windowID >= 0,"must call Glut3DWidget::create() before calling setTitle()");
      glutSetWindow(windowID);
      glutSetWindowTitle(title);
    }

    void Glut3DWidget::create(const char *title, bool fullScreen)
    {
      glutInitWindowSize( g_width, g_height );
      windowID = glutCreateWindow(title);
      activeWindow = this;
      glutDisplayFunc( glut3dDisplay );
      glutReshapeFunc( glut3dReshape );
      glutKeyboardFunc(glut3dKeyboard );
      glutSpecialFunc( glut3dSpecial );
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

    void initGLUT(int32 *ac, const char **av)
    {
      glutInit(ac, (char **) av);
      for(int i = 1; i < *ac;i++)
      {
        std::string arg(av[i]);
        printf("commandline arg: %s\n", arg.c_str());
        if (arg == "-win")
        {
          if (++i < *ac)
          {
            std::string arg2(av[i]);
            size_t pos = arg2.find("x");
            if (pos != std::string::npos)
            {
              arg2.replace(pos, 1, " ");
              std::stringstream ss(arg2);
              ss >> g_width >> g_height;
            }
          }
          else
            error("missing commandline param");
        }
        if (arg == "-vu") {
          if (!viewPortFromCmdLine) viewPortFromCmdLine = new Glut3DWidget::ViewPort;
          viewPortFromCmdLine->up.x = atof(av[i+1]);
          viewPortFromCmdLine->up.y = atof(av[i+2]);
          viewPortFromCmdLine->up.z = atof(av[i+3]);
          assert(i+3 < *ac);
          removeArgs(*ac,(char **&)av,i,4);
          --i;
          continue;
        }
        if (arg == "-vp") {
          if (!viewPortFromCmdLine) viewPortFromCmdLine = new Glut3DWidget::ViewPort;
          viewPortFromCmdLine->from.x = atof(av[i+1]);
          viewPortFromCmdLine->from.y = atof(av[i+2]);
          viewPortFromCmdLine->from.z = atof(av[i+3]);
          assert(i+3 < *ac);
          removeArgs(*ac,(char **&)av,i,4);
          --i;
          continue;
        }
        if (arg == "-vi") {
          if (!viewPortFromCmdLine) viewPortFromCmdLine = new Glut3DWidget::ViewPort;
          viewPortFromCmdLine->at.x = atof(av[i+1]);
          viewPortFromCmdLine->at.y = atof(av[i+2]);
          viewPortFromCmdLine->at.z = atof(av[i+3]);
          assert(i+3 < *ac);
          removeArgs(*ac,(char **&)av,i,4);
          --i;
          continue;
        }

        /*else if (arg == "-vp")
        {
          float x,y,z;
          if (++i < *ac)
          {
            std::stringstream ss(av[i]);
            ss >> x;
          }
          if (++i < *ac)
          {
            std::stringstream ss(av[i]);
            ss >> y;
          }
          if (++i < *ac)
          {
            std::stringstream ss(av[i]);
            ss >> z;
          }
          if (Glut3DWidget::activeWindow)
          {
            Glut3DWidget::ViewPort &cam = Glut3DWidget::activeWindow->viewPort;
            cam.from = vec3f(x,y,z);
            cam.modified = true;
          }
        }*/
      }
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

    void InspectCenter::keypress(Glut3DWidget *widget,
     int32 key)
    {
      switch(key) {
        case 'a': {
          rotate(+10.f*widget->motionSpeed,0);
        } return;
        case 'd': {
          rotate(-10.f*widget->motionSpeed,0);
        } return;
        case 'w': {
          rotate(0,+10.f*widget->motionSpeed);
        } return;
        case 's': {
          rotate(0,-10.f*widget->motionSpeed);
        } return;
      }

      Manipulator::keypress(widget,key);
    }
    void InspectCenter::rotate(float du, float dv)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      const vec3f pivot = center(widget->worldBounds);
      AffineSpace3fa xfm
      = AffineSpace3fa::translate(pivot)
      * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
      * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
      * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapUp();
      cam.modified = true;
    }

    void InspectCenter::specialkey(Glut3DWidget *widget,
     int32 key)
    {
      switch(key) {
        case GLUT_KEY_LEFT: {
          rotate(+10.f*widget->motionSpeed,0);
        } return;
        case GLUT_KEY_RIGHT: {
          rotate(-10.f*widget->motionSpeed,0);
        } return;
        case GLUT_KEY_UP: {
          rotate(0,+10.f*widget->motionSpeed);
        } return;
        case GLUT_KEY_DOWN: {
          rotate(0,-10.f*widget->motionSpeed);
        } return;
      }
      Manipulator::specialkey(widget,key);
    }

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
      // std::cout << "-------------------------------------------------------" << std::endl;
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->motionSpeed;
      float dv = (to.y - from.y) * widget->motionSpeed;

      vec2i delta_mouse = to - from;
      // PRINT(delta_mouse);

      const vec3f pivot = center(widget->worldBounds);
      AffineSpace3fa xfm
      = AffineSpace3fa::translate(pivot)
      * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
      * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
      * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapUp();
      cam.modified = true;
    }



    // ------------------------------------------------------------------
    // MOVE_MOVE manipulator - TODO.
    // ------------------------------------------------------------------

    /*! todo */
    void MoveMode::dragRight(Glut3DWidget *widget,
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

    /*! todo */
    void MoveMode::dragMiddle(Glut3DWidget *widget,
      const vec2i &to, const vec2i &from)
    {
      // it's called inspect_***CENTER*** for a reason; this class
      // will keep the rotation pivot at the center, and not do
      // anything with center mouse button...
    }

    /*! todo */
    void MoveMode::dragLeft(Glut3DWidget *widget,
      const vec2i &to, const vec2i &from)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->motionSpeed;
      float dv = (to.y - from.y) * widget->motionSpeed;

      vec2i delta_mouse = to - from;

      const vec3f pivot = center(widget->worldBounds);
      AffineSpace3fa xfm
      = AffineSpace3fa::translate(pivot)
      * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
      * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
      * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapUp();
      cam.modified = true;
    }





    void Glut3DWidget::specialkey(int32 key, const vec2f where)
    {
      if (manipulator) manipulator->specialkey(this,key);
    }
    void Glut3DWidget::keypress(char key, const vec2f where)
    {
      if (key == 'C') {
        PRINT(viewPort);
        return;
      }
      if (key == 'I' && inspectCenterManipulator) {
        manipulator = inspectCenterManipulator;
        return;
      }
      if (key == 'M' && moveModeManipulator) {
        manipulator = moveModeManipulator;
        return;
      }
      if (manipulator) manipulator->keypress(this,key);
    }




    void Manipulator::keypress(Glut3DWidget *widget, const int32 key)
    {
      if (key == 'Q') {
        exit(0);
      }
    };
    void Manipulator::specialkey(Glut3DWidget *widget, const int32 key)
    {
    };


    std::ostream &operator<<(std::ostream &o, const Glut3DWidget::ViewPort &cam)
    {
      o << "// "
        << " -vp " << cam.from.x << " " << cam.from.y << " " << cam.from.z
        << " -vi " << cam.at.x << " " << cam.at.y << " " << cam.at.z
        << " -vu " << cam.up.x << " " << cam.up.y << " " << cam.up.z
        << std::endl;
      o << "<viewPort>" << std::endl;
      o << "  <from>" << cam.from.x << " " << cam.from.y << " " << cam.from.z << "</from>" << std::endl;
      o << "  <at>" << cam.at.x << " " << cam.at.y << " " << cam.at.z << "</at>" << std::endl;
      o << "  <up>" << cam.up.x << " " << cam.up.y << " " << cam.up.z << "</up>" << std::endl;
      o << "  <aspect>" << cam.aspect << "</aspect>" << std::endl;
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
      //   float aspect;
      //   // float focalDistance;

      //   /*! viewPort frame in which the Y axis is the depth axis, and X
      //     and Z axes are parallel to the screen X and Y axis. The frame
      //     itself remains normalized. */
      //   AffineSpace3fa frame;

      //   /*! set 'up' vector. if this vector is '0,0,0' the viewer will
      //    *not* apply the up-vector after viewPort manipulation */
      //   void snapUp();



  }
}

