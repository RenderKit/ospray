#include "glut3D.h"
#ifdef __APPLE__
#include "GLUT/glut.h"
#else
#include "GL/glut.h"
#endif

namespace ospray {
  
  /*! currently active window */
  Glut3DWidget *Glut3DWidget::activeWindow = NULL;
  Glut3DWidget::InspectCenter Glut3DWidget::INSPECT_CENTER;


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
  // implementation of glut3d::cameras
  // ------------------------------------------------------------------
  Glut3DWidget::Camera::Camera()
    : from(0,-1,0),
      at(0,0,0),
      up(0,0,1),
      aspectRatio(1.f),
      openingAngle(60.f*M_PI/360.f),
      modified(true)
  {
    frame = AffineSpace3f::translate(from) * AffineSpace3f(embree::one);
  }

  void Glut3DWidget::Camera::snapUp()
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
    if (camera.modified)
      forceRedraw();
  }
  
  Glut3DWidget::Glut3DWidget(FrameBufferMode frameBufferMode, Manipulator *manipulator)
    : cameraSpeed(.003f),
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
    camera.up = up;
    if (up != vec3f(0.f)) {
      camera.snapUp();
      forceRedraw();
    }
  }

  void Glut3DWidget::reshape(const vec2i &newSize) 
  { 
    windowSize = newSize; 
    camera.aspectRatio = newSize.x/float(newSize.y);

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
    vec3f diag   = size(worldBounds);
    diag         = max(diag,vec3f(0.3f*length(diag)));
    vec3f from   = center - .75f*vec3f(-.6*diag.x,-1.2*diag.y,.8*diag.z);
    vec3f dir    = center - from;
    vec3f up     = camera.up;

    camera.at    = center;
    camera.from  = from;
    camera.up    = up;

    if (length(up) < 1e-3f)
      up = vec3f(0,0,1.f);

    this->worldBounds = worldBounds;
    camera.frame.l.vy = normalize(dir);
    camera.frame.l.vx = normalize(cross(camera.frame.l.vy,up));
    camera.frame.l.vz = normalize(cross(camera.frame.l.vx,camera.frame.l.vy));
    camera.frame.p    = from;
    camera.snapUp();
    camera.modified = true;
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

  void Glut3DWidget::runGlut()
  {
    glutMainLoop();
  }

  void Glut3DWidget::initGlut(int *ac, const char **av)
  {
    glutInit(ac,(char **)av);
  }

  // ------------------------------------------------------------------
  // base manipulator
  // ------------------------------------------------------------------
  void Glut3DWidget::Manipulator::motion(Glut3DWidget *widget)
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

  /*! INSPECT_CENTER::RightButton: move lookfrom/camera positoin
      forward/backward on right mouse button */
  void Glut3DWidget::InspectCenter::dragRight(Glut3DWidget *widget, 
                                              const vec2i &to, const vec2i &from) 
  {
    Glut3DWidget::Camera &cam = widget->camera;
    float fwd = (to.y - from.y) * 4 * widget->cameraSpeed
      * length(size(widget->worldBounds));
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
  void Glut3DWidget::InspectCenter::dragMiddle(Glut3DWidget *widget, 
                                               const vec2i &to, const vec2i &from) 
  {
    Glut3DWidget::Camera &cam = widget->camera;
    float fwd = (to.y - from.y) * 4 * widget->cameraSpeed
      * length(size(widget->worldBounds));
    float oldDist = length(cam.at - cam.from);
    float newDist = oldDist - fwd;
    if (newDist < 1e-3f) 
      return;
    cam.at = cam.from + newDist * cam.frame.l.vy;
    cam.modified = true;
  }

  /*! INSPECT_CENTER::LeftButton: rotate camera position
      ('camera.from') around center of world bounds */
  void Glut3DWidget::InspectCenter::dragLeft(Glut3DWidget *widget, 
                                             const vec2i &to, const vec2i &from) 
  {
    Glut3DWidget::Camera &cam = widget->camera;
    float du = (to.x - from.x) * widget->cameraSpeed;
    float dv = (to.y - from.y) * widget->cameraSpeed;

    const vec3f pivot = center(widget->worldBounds);
    AffineSpace3f xfm 
      = AffineSpace3f::translate(pivot)
      * AffineSpace3f::rotate(cam.frame.l.vx,dv)
      * AffineSpace3f::rotate(cam.frame.l.vz,du)
      * AffineSpace3f::translate(-pivot);
    cam.frame = xfm * cam.frame;
    cam.from  = xfmPoint(xfm,cam.from);
    cam.at    = xfmPoint(xfm,cam.at);
    cam.snapUp();
    cam.modified = true;
  }

}

