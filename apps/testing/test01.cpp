#include "../util/glut3D/glut3D.h"

/*! \file test01 Test Glut 3D widget 

  Test test mainly stretches correctness of all the infrastructure to
  compile and link with other ospray components. This example will use
  the glut3D viewer widget, but not do anything useful (other than
  opening a blank window) when called.
 
 */
int main(int ac, const char **av)
{
  ospray::glut3D::initGLUT(&ac,av);

  ospray::glut3D::Glut3DWidget 
    widget(ospray::glut3D::Glut3DWidget::FRAMEBUFFER_FLOAT,
           &ospray::glut3D::Glut3DWidget::INSPECT_CENTER);
  widget.create("OSPRay glut3D viewer test app");

  printf("Opening a simple ospray Glut3DWidget.");
  printf("This will not do anything other than display a blank window; press 'Q' to quit.\n");
  ospray::glut3D::runGLUT();
}
