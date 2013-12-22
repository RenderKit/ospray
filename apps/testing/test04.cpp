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
      fb(NULL), renderer(NULL), model(NULL)
  {
    renderer = ospNewRenderer("test_screen");
    Assert(renderer != NULL && "could not create renderer");
  };
  virtual void reshape(const ospray::vec2i &newSize) 
  {
    PING; PRINT(newSize);
    Glut3DWidget::reshape(newSize);
    if (fb) ospFreeFrameBuffer(fb);
    fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
  }

  virtual void display() 
  {
    if (!fb || !renderer) return;

    fps.startRender();

    ucharFB = (unsigned int *)ospMapFrameBuffer(fb);
    ospUnmapFrameBuffer(ucharFB,fb);
    glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, ucharFB);
    ospRenderFrame(fb,renderer,model);
    //    glutSwapBuffers();
    
    Glut3DWidget::display();
    fps.doneRender();
    char title[1000];
    sprintf(title,"Test04: GlutWidget+ospray API rest (%f fps)",
            fps.getFPS());
    setTitle(title);
    glutPostRedisplay();
  }

  OSPModel       model;
  OSPFrameBuffer fb;
  OSPRenderer    renderer;
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
