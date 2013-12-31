/*! \file test03 Test Glut 3D Widget and ospray FrameBuffer 

 */

#include "../util/glut3D/glut3D.h"
#include "GL/glut.h"
#include "rtcore.h"

using ospray::glut3D::Glut3DWidget;

extern "C" {
  void ispc_renderFrame(ospray::vec3fa *fb,
                        int size_x, 
                        int size_y,
                        int frameID);
}

struct MainWindow : public Glut3DWidget {
  MainWindow() 
    : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_FLOAT),
      frameID(0)
  {};
  virtual void display() 
  {
    fps.startRender();

    ispc_renderFrame(floatFB,windowSize.x,windowSize.y,frameID);
    ++frameID;
    Glut3DWidget::display();
    fps.doneRender();
    char title[1000];
    sprintf(title,"Test03: GlutWidget frame buffer test (%f fps)",
            fps.getFPS());
    setTitle(title);
    glutPostRedisplay();
  }
  int frameID;
  ospray::glut3D::FPSCounter fps;
};

int main(int ac, const char **av)
{
  ospray::glut3D::initGLUT(&ac,av);
  rtcInit(NULL);
  MainWindow window;
  window.create("OSPRay glut3D viewer test app");

  printf("Opening a simple ospray Glut3DWidget, and tests rendering via a simple test frame.\n");
  printf("Press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
