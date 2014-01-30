/*! \file test03 Test Glut 3D Widget and (dummy-)rendering through the ospray api

 */

#include "../util/glut3D/glut3D.h"
#include "GL/glut.h"
#include "rtcore.h"
#include "ospray/ospray.h"

using ospray::glut3D::Glut3DWidget;

struct MainWindow : public Glut3DWidget {
  MainWindow() 
    : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
      fb(NULL), renderer(NULL), model(NULL)
  {
    renderer = ospNewRenderer("test_screen");
    Assert(renderer != NULL && "could not create renderer");
  };
  virtual void reshape(const ospray::vec2i &newSize) 
  {
    Glut3DWidget::reshape(newSize);
    if (fb) ospFreeFrameBuffer(fb);
    fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
  }

  virtual void display() 
  {
    if (!fb || !renderer) return;

    fps.startRender();
    ospRenderFrame(fb,renderer);
    fps.doneRender();
    
    ucharFB = (uint32 *) ospMapFrameBuffer(fb);
    frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
    Glut3DWidget::display();
    
    ospUnmapFrameBuffer(ucharFB,fb);
    
    char title[1000];
    sprintf(title,"Test04: GlutWidget+ospray API rest (%f fps)",
            fps.getFPS());
    setTitle(title);
    forceRedraw();
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
  MainWindow window;
  window.create("OSPRay glut3D viewer test app");

  printf("Opening a simple ospray Glut3DWidget, and tests (dummy-)rendering via the ospray api.\n");
  printf("Press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
