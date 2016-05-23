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

#include "glut3D.h"
#ifdef __APPLE__
#include "GLUT/glut.h"
#else
#include "GL/glut.h"
#endif

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h> // for Sleep
#  define _USE_MATH_DEFINES
#  include <math.h> // M_PI
#else
#  include <sys/times.h>
#  include <unistd.h> // for usleep
#endif
#include <sstream>

namespace ospray {

  namespace glut3D {

    bool dumpScreensDuringAnimation = false;

    FPSCounter::FPSCounter()
    {
      smooth_nom = 0.;
      smooth_den = 0.;
      frameStartTime = 0.;
    }
    
    void FPSCounter::startRender() 
    { 
      frameStartTime = ospcommon::getSysTime(); 
    }
    
    void FPSCounter::doneRender() {
      double seconds = ospcommon::getSysTime() - frameStartTime; 
      smooth_nom = smooth_nom * 0.8f + seconds;
      smooth_den = smooth_den * 0.8f + 1.f;
    }

    /*! write given frame buffer to file, in PPM P6 format. */
    void saveFrameBufferToFile(const char *fileName,
                               const uint32_t *pixel,
                               const uint32_t sizeX, const uint32_t sizeY)
    {
      FILE *file = fopen(fileName,"wb");
      if (!file) {
        std::cerr << "#osp:glut3D: Warning - could not create screen shot file '" 
                  << fileName << "'" << std::endl;
        return;
      }
      fprintf(file,"P6\n%i %i\n255\n",sizeX,sizeY);
      unsigned char *out = (unsigned char *)alloca(3*sizeX);
      for (int y=0;y<sizeY;y++) {
        const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
        for (int x=0;x<sizeX;x++) {
          out[3*x+0] = in[4*x+0];
          out[3*x+1] = in[4*x+1];
          out[3*x+2] = in[4*x+2];
        }
        fwrite(out, 3*sizeX, sizeof(char), file);
      }
      fprintf(file,"\n");
      fclose(file);
      std::cout << "#osp:glut3D: saved framebuffer to file " << fileName << std::endl;
    }

#define INVERT_RMB 
    /*! currently active window */
    Glut3DWidget *Glut3DWidget::activeWindow = NULL;
    vec2i Glut3DWidget::defaultInitSize(1024,768);

    bool animating = false;

    // InspectCenter Glut3DWidget::INSPECT_CENTER;
    /*! viewport as specified on the command line */
    Glut3DWidget::ViewPort *viewPortFromCmdLine = NULL;
    vec3f upVectorFromCmdLine(0,1,0);

    // ------------------------------------------------------------------
    // glut event handlers
    // ------------------------------------------------------------------

    void glut3dReshape(int32_t x, int32_t y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->reshape(vec2i(x,y));
    }

    void glut3dDisplay( void )
    {
      if (animating && Glut3DWidget::activeWindow && Glut3DWidget::activeWindow->inspectCenterManipulator) {
        InspectCenter *hack = (InspectCenter *) Glut3DWidget::activeWindow->inspectCenterManipulator;
        hack->rotate(-10.f * Glut3DWidget::activeWindow->motionSpeed, 0);
      }
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->display();
    }

    void glut3dKeyboard(unsigned char key, int32_t x, int32_t y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->keypress(key,vec2i(x,y));
    }
    void glut3dSpecial(int32_t key, int32_t x, int32_t y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->specialkey(key,vec2i(x,y));
    }

    void glut3dIdle( void )
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->idle();
    }
    void glut3dMotionFunc(int32_t x, int32_t y)
    {
      if (Glut3DWidget::activeWindow)
        Glut3DWidget::activeWindow->motion(vec2i(x,y));
    }

    void glut3dMouseFunc(int32_t whichButton, int32_t released, int32_t x, int32_t y)
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
        up(upVectorFromCmdLine),
        aspect(1.f),
        openingAngle(60.f*M_PI/360.f),
        modified(true)
    {
      frame = AffineSpace3fa::translate(from) * AffineSpace3fa(ospcommon::one);
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
    void Glut3DWidget::mouseButton(int32_t whichButton, bool released, const vec2i &pos)
    {

      if (pos != currMousePos)
        motion(pos);
      lastButtonState = currButtonState;

      if (released)
        currButtonState = currButtonState & ~(1<<whichButton);
      else
        currButtonState = currButtonState |  (1<<whichButton);
      currModifiers = glutGetModifiers();

      manipulator->button(this, pos);
    }

    void Glut3DWidget::motion(const vec2i &pos)
    {
      currMousePos = pos;
      if (currButtonState != lastButtonState) {
        // some button got pressed; reset 'old' pos to new pos.
        lastMousePos = currMousePos;
        lastButtonState = currButtonState;
      }

      manipulator->motion(this);
      lastMousePos = currMousePos;
      if (viewPort.modified)
        forceRedraw();
    }

    Glut3DWidget::Glut3DWidget(FrameBufferMode frameBufferMode,
                               ManipulatorMode initialManipulator,
                               int allowedManipulators)
      : motionSpeed(.003f),rotateSpeed(.003f),
        windowID(-1),
        lastMousePos(-1,-1),
        currMousePos(-1,-1),
        windowSize(-1,-1),
        currModifiers(0),
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
        computeFrame();
      }
    }

    void Glut3DWidget::computeFrame()
    {
      viewPort.frame.l.vy = normalize(viewPort.at - viewPort.from);
      viewPort.frame.l.vx = normalize(cross(viewPort.frame.l.vy,viewPort.up));
      viewPort.frame.l.vz = normalize(cross(viewPort.frame.l.vx,viewPort.frame.l.vy));
      viewPort.frame.p    = viewPort.from;
      viewPort.snapUp();
      viewPort.modified = true;
    }

    void Glut3DWidget::setZUp(const vec3f &up)
    {
      viewPort.up = up;
      if (up != vec3f(0.f)) {
        viewPort.snapUp();
        forceRedraw();
      }
    }

    void Glut3DWidget::idle()
    {
#ifdef _WIN32
      Sleep(1);
#else
      usleep(1000);
#endif
    }

    void Glut3DWidget::reshape(const vec2i &newSize)
    {
      windowSize = newSize;
      viewPort.aspect = newSize.x/float(newSize.y);
      glViewport(0, 0, windowSize.x, windowSize.y);

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
#ifndef _WIN32
        if (animating && dumpScreensDuringAnimation) {
          char tmpFileName[] = "/tmp/ospray_scene_dump_file.XXXXXXXXXX";
          static const char *dumpFileRoot;
          if (!dumpFileRoot) 
            dumpFileRoot = getenv("OSPRAY_SCREEN_DUMP_ROOT");
          if (!dumpFileRoot) {
            mkstemp(tmpFileName);
            dumpFileRoot = tmpFileName;
          }

          char fileName[100000];
          sprintf(fileName,"%s_%08ld.ppm",dumpFileRoot,times(NULL));
          saveFrameBufferToFile(fileName,ucharFB,windowSize.x,windowSize.y);
        }
#endif
      } else if (frameBufferMode == Glut3DWidget::FRAMEBUFFER_FLOAT && floatFB) {
        glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_FLOAT, floatFB);
      } else {
        glClearColor(0.f,0.f,0.f,1.f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      }
      glutSwapBuffers();
    }

    void Glut3DWidget::clearPixels()
    {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glutSwapBuffers();
    }

    void Glut3DWidget::drawPixels(const uint32_t *framebuffer)
    {
      throw std::runtime_error("should not be used right now");
      glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
      glutSwapBuffers();
    }

    void Glut3DWidget::drawPixels(const vec3fa *framebuffer)
    {
      throw std::runtime_error("should not be used right now");
      glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_FLOAT, framebuffer);
      glutSwapBuffers();
    }

    void Glut3DWidget::setViewPort(const vec3f from, 
                                   const vec3f at,
                                   const vec3f up)
    {
      const vec3f dir = at - from;
      viewPort.at    = at;
      viewPort.from  = from;
      viewPort.up    = up;

      this->worldBounds = worldBounds;
      viewPort.frame.l.vy = normalize(dir);
      viewPort.frame.l.vx = normalize(cross(viewPort.frame.l.vy,up));
      viewPort.frame.l.vz = normalize(cross(viewPort.frame.l.vx,viewPort.frame.l.vy));
      viewPort.frame.p    = from;
      viewPort.snapUp();
      viewPort.modified = true;
    }

    void Glut3DWidget::setWorldBounds(const box3f &worldBounds)
    {
      vec3f center = ospcommon::center(worldBounds);
      vec3f diag   = worldBounds.size();
      diag         = max(diag,vec3f(0.3f*length(diag)));
      vec3f from   = center - .75f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
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
      motionSpeed = length(diag) * .001f;
      // std::cout << "glut3d: setting world bounds " << worldBounds << ", motion speed " << motionSpeed << std::endl;
    }

    void Glut3DWidget::setTitle(const char *title)
    {
      Assert2(windowID >= 0,"must call Glut3DWidget::create() before calling setTitle()");
      glutSetWindow(windowID);
      glutSetWindowTitle(title);
    }

    void Glut3DWidget::create(const char *title, 
                              const vec2i &size,
                              bool fullScreen)
    {
      glutInitWindowSize( size.x, size.y );
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

    void initGLUT(int32_t *ac, const char **av)
    {
      bool useWindow = true;
      for (int i = 1; i < *ac;i++){
          std::string arg(av[i]);
          if (arg == "--nowin") {
            useWindow = false;
          }
      }

      if (useWindow) {
        glutInit(ac, (char **) av);
        glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE);
      }

      for(int i = 1; i < *ac;i++)
        {
          std::string arg(av[i]);
          if (arg == "-win") {
            std::string arg2(av[i+1]);
            size_t pos = arg2.find("x");
            if (pos != std::string::npos) {
              arg2.replace(pos, 1, " ");
              std::stringstream ss(arg2);
              ss >> Glut3DWidget::defaultInitSize.x >> Glut3DWidget::defaultInitSize.y;
              removeArgs(*ac,(char **&)av,i,2); --i;
            } else {
              Glut3DWidget::defaultInitSize.x = atoi(av[i+1]);
              Glut3DWidget::defaultInitSize.y = atoi(av[i+1]);
              removeArgs(*ac,(char **&)av,i,3); --i;
            }
            continue;
          }
          if (arg == "--1k" || arg == "-1k") {
            Glut3DWidget::defaultInitSize.x = Glut3DWidget::defaultInitSize.y = 1024;
            removeArgs(*ac,(char **&)av,i,1); --i;
            continue;
          }
          if (arg == "--size") {
            Glut3DWidget::defaultInitSize.x = atoi(av[i+1]);
            Glut3DWidget::defaultInitSize.y = atoi(av[i+2]);
            removeArgs(*ac,(char **&)av,i,3); --i;
            continue;
          }
          if (arg == "-vu") {
            upVectorFromCmdLine.x = atof(av[i+1]);
            upVectorFromCmdLine.y = atof(av[i+2]);
            upVectorFromCmdLine.z = atof(av[i+3]);
            if (viewPortFromCmdLine) 
              viewPortFromCmdLine->up = upVectorFromCmdLine;
            assert(i+3 < *ac);
            removeArgs(*ac,(char **&)av,i,4); --i;
            continue;
          }
          if (arg == "-vp") {
            if (!viewPortFromCmdLine) viewPortFromCmdLine = new Glut3DWidget::ViewPort;
            viewPortFromCmdLine->from.x = atof(av[i+1]);
            viewPortFromCmdLine->from.y = atof(av[i+2]);
            viewPortFromCmdLine->from.z = atof(av[i+3]);
            assert(i+3 < *ac);
            removeArgs(*ac,(char **&)av,i,4); --i;
            continue;
          }
          if (arg == "-vi") {
            if (!viewPortFromCmdLine) viewPortFromCmdLine = new Glut3DWidget::ViewPort;
            viewPortFromCmdLine->at.x = atof(av[i+1]);
            viewPortFromCmdLine->at.y = atof(av[i+2]);
            viewPortFromCmdLine->at.z = atof(av[i+3]);
            assert(i+3 < *ac);
            removeArgs(*ac,(char **&)av,i,4); --i;
            continue;
          }
        }
    }

    // ------------------------------------------------------------------
    // base manipulator
    // ------------------------------------------------------------------
    void Manipulator::motion(Glut3DWidget *widget)
    {
      if ((widget->currButtonState == (1<<GLUT_RIGHT_BUTTON))
          ||
          ((widget->currButtonState == (1<<GLUT_LEFT_BUTTON)) 
           && 
           (widget->currModifiers & GLUT_ACTIVE_ALT))
          ) {
        dragRight(widget,widget->currMousePos,widget->lastMousePos);
      } else if ((widget->currButtonState == (1<<GLUT_MIDDLE_BUTTON)) 
                 ||
                 ((widget->currButtonState == (1<<GLUT_LEFT_BUTTON)) 
                  && 
                  (widget->currModifiers & GLUT_ACTIVE_CTRL))
                 ) {
        dragMiddle(widget,widget->currMousePos,widget->lastMousePos);
      } else if (widget->currButtonState == (1<<GLUT_LEFT_BUTTON)) {
        dragLeft(widget,widget->currMousePos,widget->lastMousePos);
      } 
    }

    // ------------------------------------------------------------------
    // INSPECT_CENTER manipulator
    // ------------------------------------------------------------------
    InspectCenter::InspectCenter(Glut3DWidget *widget)
      : Manipulator(widget)
      , pivot(ospcommon::center(widget->worldBounds))
    {}

    void InspectCenter::keypress(Glut3DWidget *widget,
                                 int32_t key)
    {
      switch(key) {
      case 'a': {
        rotate(+10.f*widget->rotateSpeed,0);
      } return;
      case 'd': {
        rotate(-10.f*widget->rotateSpeed,0);
      } return;
      case 'w': {
        rotate(0,+10.f*widget->rotateSpeed);
      } return;
      case 's': {
        rotate(0,-10.f*widget->rotateSpeed);
      } return;
      }

      Manipulator::keypress(widget,key);
    }

    void InspectCenter::button(Glut3DWidget *widget, const vec2i &pos)
    {
    }

    void InspectCenter::rotate(float du, float dv)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      const vec3f pivot = widget->viewPort.at;//center(widget->worldBounds);
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
                                   int32_t key)
    {
      switch(key) {
      case GLUT_KEY_LEFT: {
        rotate(+10.f*widget->rotateSpeed,0);
      } return;
      case GLUT_KEY_RIGHT: {
        rotate(-10.f*widget->rotateSpeed,0);
      } return;
      case GLUT_KEY_UP: {
        rotate(0,+10.f*widget->rotateSpeed);
      } return;
      case GLUT_KEY_DOWN: {
        rotate(0,-10.f*widget->rotateSpeed);
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
      float fwd = 
#ifdef INVERT_RMB
#else
        - 
#endif
        (to.y - from.y) * 4 * widget->motionSpeed;
      // * length(widget->worldBounds.size());
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
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x);
      float dv = (to.y - from.y);

      vec2i delta_mouse = (to - from);

      AffineSpace3fa xfm = AffineSpace3fa::translate( widget->motionSpeed * dv * cam.frame.l.vz )
        * AffineSpace3fa::translate( -1.0f * widget->motionSpeed * du * cam.frame.l.vx );

      cam.frame = xfm * cam.frame;
      cam.from = xfmPoint(xfm, cam.from);
      cam.at = xfmPoint(xfm, cam.at);
      cam.modified = true;
    }

    void InspectCenter::dragLeft(Glut3DWidget *widget,
                                 const vec2i &to, const vec2i &from)
    {
      // std::cout << "-------------------------------------------------------" << std::endl;
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->rotateSpeed;
      float dv = (to.y - from.y) * widget->rotateSpeed;

      vec2i delta_mouse = to - from;

      const vec3f pivot = cam.at; //center(widget->worldBounds);
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

    /*! \brief key press events for move mode

      Right now, recognizes the following move modes:

      - w : move forward
      - s : move backward
      - a : pan left
      - d : pan right

    */
    void MoveMode::keypress(Glut3DWidget *widget,
                            int32_t key)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      switch(key) {
      case 'w': {
        float fwd = 8 * widget->motionSpeed;
        cam.from = cam.from + fwd * cam.frame.l.vy;
        cam.at   = cam.at   + fwd * cam.frame.l.vy;
        cam.frame.p = cam.from;
        cam.modified = true;
      } return;
      case 's': {
        float fwd = 8 * widget->motionSpeed;
        cam.from = cam.from - fwd * cam.frame.l.vy;
        cam.at   = cam.at   - fwd * cam.frame.l.vy;
        cam.frame.p = cam.from;
        cam.modified = true;
      } return;
      case 'd': {
        float fwd = 8 * widget->motionSpeed;
        cam.from = cam.from + fwd * cam.frame.l.vx;
        cam.at   = cam.at   + fwd * cam.frame.l.vx;
        cam.frame.p = cam.from;
        cam.modified = true;
      } return;
      case 'a': {
        float fwd = 8 * widget->motionSpeed;
        cam.from = cam.from - fwd * cam.frame.l.vx;
        cam.at   = cam.at   - fwd * cam.frame.l.vx;
        cam.frame.p = cam.from;
        cam.modified = true;
      } return;
      }
      Manipulator::keypress(widget,key);
    }

    void MoveMode::dragRight(Glut3DWidget *widget,
                             const vec2i &to, const vec2i &from)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float fwd = 
#ifdef INVERT_RMB
#else
        - 
#endif
        (to.y - from.y) * 4 * widget->motionSpeed;
      cam.from = cam.from + fwd * cam.frame.l.vy;
      cam.at   = cam.at   + fwd * cam.frame.l.vy;
      cam.frame.p = cam.from;
      cam.modified = true;
    }

    /*! todo */
    void MoveMode::dragMiddle(Glut3DWidget *widget,
                              const vec2i &to, const vec2i &from)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x);
      float dv = (to.y - from.y);

      vec2i delta_mouse = (to - from);

      AffineSpace3fa xfm = AffineSpace3fa::translate( widget->motionSpeed * dv * cam.frame.l.vz )
        * AffineSpace3fa::translate( -1.0f * widget->motionSpeed * du * cam.frame.l.vx );

      cam.frame = xfm * cam.frame;
      cam.from = xfmPoint(xfm, cam.from);
      cam.at = xfmPoint(xfm, cam.at);
      cam.modified = true;
    }

    void MoveMode::dragLeft(Glut3DWidget *widget,
                            const vec2i &to, const vec2i &from)
    {
      Glut3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->rotateSpeed;
      float dv = (to.y - from.y) * widget->rotateSpeed;

      vec2i delta_mouse = to - from;

      const vec3f pivot = cam.from; //center(widget->worldBounds);
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

    void Glut3DWidget::specialkey(int32_t key, const vec2i &where)
    {
      if (manipulator) manipulator->specialkey(this,key);
    }
    void Glut3DWidget::keypress(char key, const vec2i &where)
    {
      if (key == '!') {
        if (animating) {
          dumpScreensDuringAnimation = !dumpScreensDuringAnimation;
        } else {
          char tmpFileName[] = "/tmp/ospray_screen_dump_file.XXXXXXXX";
          static const char *dumpFileRoot;
          if (!dumpFileRoot) 
            dumpFileRoot = getenv("OSPRAY_SCREEN_DUMP_ROOT");
#ifndef _WIN32
          if (!dumpFileRoot) {
            mkstemp(tmpFileName);
            dumpFileRoot = tmpFileName;
          }
#endif
          char fileName[100000];
          static int frameDumpSequenceID = 0;
          sprintf(fileName,"%s_%05d.ppm",dumpFileRoot,frameDumpSequenceID++);
          if (ucharFB)
            saveFrameBufferToFile(fileName,ucharFB,windowSize.x,windowSize.y);
          return;
        }
      } 

      if (key == 'C') {
        PRINT(viewPort);
        return;
      }
      if (key == 'I' && inspectCenterManipulator) {
        // 'i'nspect mode
        manipulator = inspectCenterManipulator;
        return;
      }
      if ((key == 'M' || key == 'F') && moveModeManipulator) {
        manipulator = moveModeManipulator;
        return;
      }
      if (key == 'F' && moveModeManipulator) {
        // 'f'ly mode
        manipulator = moveModeManipulator;
        return;
      }
      if (key == 'A' && inspectCenterManipulator) {
        animating = !animating;
        return;
      }
      if (key == '+') { 
        motionSpeed *= 1.5f; 
        std::cout << "glut3d: new motion speed " << motionSpeed << std:: endl;
        return; 
      }
      if (key == '-') { 
        motionSpeed /= 1.5f; 
        std::cout << "glut3d: new motion speed " << motionSpeed << std:: endl;
        return; 
      }
      if (manipulator) manipulator->keypress(this,key);
    }




    void Manipulator::keypress(Glut3DWidget *widget, const int32_t key)
    {
      switch(key) {
      case 27 /*ESC*/:
      case 'q':
      case 'Q':
        _exit(0);
      }
    };
    void Manipulator::specialkey(Glut3DWidget *widget, const int32_t key)
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
  }
}

