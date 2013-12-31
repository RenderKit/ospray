/*! \file test02 Test Glut 3D Widget and ospray FrameBuffer 

 */

#include "../util/glut3D/glut3D.h"
#include "GL/glut.h"

using ospray::glut3D::Glut3DWidget;

struct MainWindow : public Glut3DWidget {
  MainWindow() 
    : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_FLOAT),
      frameID(0)
  {};
  virtual void display() 
  {
    fps.startRender();
    ospray::vec3fa *ptr = this->floatFB;
#pragma openmp parallel for
    for (int y=0;y<windowSize.y;y++)
#pragma vector
      for (int x=0;x<windowSize.x;x++) {
        ospray::vec3fa v;
        v.x = float((x+frameID)%windowSize.x)/windowSize.x;
        v.y = float((y+frameID)%windowSize.y)/windowSize.y;
        v.z = float(x+y)/(windowSize.x+windowSize.y);
        v.w = 0.f;
        *ptr++ = v;
      }
    ++frameID;
    Glut3DWidget::display();
    fps.doneRender();
    char title[1000];
    sprintf(title,"Test02: GlutWidget frame buffer test (%f fps)",
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

  MainWindow window;
  window.create("OSPRay glut3D viewer test app");

  printf("Opening a simple ospray Glut3DWidget, and tests rendering via a simple test frame.\n");
  printf("Press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
