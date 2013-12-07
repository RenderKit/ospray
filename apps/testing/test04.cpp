/*! \file test03 Test Glut 3D Widget and (dummy-)rendering through the ospray api

 */

#include "../util/glut3D/glut3D.h"
#include "GL/glut.h"
#include "rtcore.h"
#include "ospray/ospray.h"

using ospray::glut3D::Glut3DWidget;

struct MainWindow : public Glut3DWidget {
  MainWindow() 
    : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE,
                   &Glut3DWidget::INSPECT_CENTER),
      frameID(0), fb(NULL)
  {};
  virtual void reshape(const ospray::vec2i &newSize) 
  {
    PING; PRINT(newSize);
    Glut3DWidget::reshape(newSize);
    if (fb) ospFrameBufferDestroy(fb);
    fb = ospFrameBufferCreate(newSize,OSP_RGBA_I8);
  }

  virtual void display() 
  {
    if (!fb) return;

    fps.startRender();
    
    ucharFB = (unsigned int *)ospFrameBufferMap(fb);
    ospFrameBufferUnmap(ucharFB,fb);
    glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, ucharFB);
    
    printf("should be rendering here...\n");
    glutSwapBuffers();
    
    ++frameID;
    Glut3DWidget::display();
    fps.doneRender();
    char title[1000];
    sprintf(title,"Test04: GlutWidget+ospray API rest (%f fps)",
            fps.getFPS());
    setTitle(title);
    glutPostRedisplay();
  }

  OSPFrameBuffer fb;
  int frameID;
  ospray::glut3D::FPSCounter fps;
};

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  rtcInit(NULL);
  MainWindow window;
  window.create("OSPRay glut3D viewer test app");

  printf("Opening a simple ospray Glut3DWidget, and tests (dummy-)rendering via the ospray api.\n");
  printf("Press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
