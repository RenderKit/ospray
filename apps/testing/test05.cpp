/*! \file test03 Test Glut 3D Widget and (dummy-)rendering through the ospray api

 */

#include "../util/glut3D/glut3D.h"
#include "rtcore.h"
#include "ospray/ospray.h"
#include "ospray/geometry/trianglemesh.h"

using ospray::glut3D::Glut3DWidget;

struct MainWindow : public Glut3DWidget {
  MainWindow() 
    : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
      fb(NULL), renderer(NULL), model(NULL)
  {
  };
  virtual void reshape(const ospray::vec2i &newSize) 
  {
    Glut3DWidget::reshape(newSize);
    if (fb) ospFreeFrameBuffer(fb);
    fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
  }

  void createRenderer()
  {
      camera = ospNewCamera("perspective");
      Assert(camera != NULL && "could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);

    // OSPModel    ospModel
    model = ospNewModel();
    OSPGeometry ospMesh  = (OSPGeometry)ospray::makeArrow();
    ospAddGeometry(model,ospMesh);
    ospCommit(model);
    
    PING;

    renderer = ospNewRenderer("raycast");
    Assert(renderer != NULL && "could not create renderer");

    PRINT((void*)renderer);
    ospSetParam(renderer,"world",model);
    ospSetParam(renderer,"camera",camera);
    ospCommit(renderer);

    PING;
  }
  virtual void display() 
  {
    if (!fb) return;

    if (!renderer) createRenderer();

      if (viewPort.modified) {
        Assert2(camera,"ospray camera is null");
        // PRINT(viewPort);
        ospSetVec3f(camera,"pos",viewPort.from);
        ospSetVec3f(camera,"dir",viewPort.at-viewPort.from);
        ospSetVec3f(camera,"up",viewPort.up);
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
        viewPort.modified = false;
      }


    fps.startRender();
    ospRenderFrame(fb,renderer);
    fps.doneRender();
    
    ucharFB = (unsigned int *)ospMapFrameBuffer(fb);
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
  OSPCamera      camera;
  ospray::glut3D::FPSCounter fps;
};

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  MainWindow window;
  window.create("OSPRay glut3D viewer test app");
  window.setWorldBounds(ospray::box3f(ospray::vec3f(-1.f),ospray::vec3f(+1.f)));
  printf("Opening a simple ospray Glut3DWidget, and tests (dummy-)rendering via the ospray api.\n");
  printf("Press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
