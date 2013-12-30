/*! \file objView.cpp A GLUT-based viewer for Wavefront OBJ files */

// viewer widget
#include "../util/glut3D/glut3D.h"
#include "../util/miniSG/miniSG.h"
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;

  Ref<miniSG::Model> msgModel = NULL;
  OSPModel           ospModel = NULL;

  void error(const std::string &msg)
  {
    cout << "ospray::stlViewer fatal error : " << msg << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./stlViewer <inFileName>" << endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;

  struct MSGViewer : public Glut3DWidget {
    MSGViewer(OSPModel model) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE,
                     &Glut3DWidget::INSPECT_CENTER),
        fb(NULL), renderer(NULL), model(model)
    {
      Assert(model && "null model handle");
      renderer = ospNewRenderer("ray_cast");
      ospSetParam(renderer,"world",model);
      Assert(renderer != NULL && "could not create renderer");
    };
    virtual void reshape(const ospray::vec2i &newSize) 
    {
      PING;
      Glut3DWidget::reshape(newSize);
      PING;
      if (fb) ospFreeFrameBuffer(fb);
      PING;
      fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
      PING;
    }

    virtual void display() 
    {
      if (!fb || !renderer) return;

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
    ospray::glut3D::FPSCounter fps;
  };

  void stlViewerMain(int &ac, const char **&av)
  {
    msgModel = new miniSG::Model;

    // -------------------------------------------------------
    // parse cmdline
    // -------------------------------------------------------
    cout << "msgView: starting to process cmdline arguments" << endl;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (av[i][0] == '-') {

        error("unkown commandline argument '"+arg+"'");
      } else {
        embree::FileName fn = arg;
        if (fn.ext() == "stl")
          miniSG::importSTL(*msgModel,fn);
        else if (fn.ext() == "msg")
          miniSG::importMSG(*msgModel,fn);
        else if (fn.ext() == "obj")
          miniSG::importOBJ(*msgModel,fn);
        else
          error("unrecognized file format in filename '"+arg+"'");
      }
    }
    // -------------------------------------------------------
    // done parsing
    // -------------------------------------------------------
    cout << "msgView: done parsing. found model with" << endl;
    cout << "  - num materials: " << msgModel->material.size() << endl;
    cout << "  - num meshes   : " << msgModel->mesh.size() << " ";
    int numUniqueTris = 0;
    int numInstancedTris = 0;
    for (int i=0;i<msgModel->mesh.size();i++) {
      cout << "[" << msgModel->mesh[i]->size() << "]";
      numUniqueTris += msgModel->mesh[i]->size();
    }
    cout << endl;
    cout << "  - num instances: " << msgModel->instance.size() << " ";
    for (int i=0;i<msgModel->mesh.size();i++) {
      cout << "[" << msgModel->mesh[msgModel->instance[i].meshID]->size() << "]";
      numInstancedTris += msgModel->mesh[msgModel->instance[i].meshID]->size();
    }
    cout << endl;
    cout << "  - num unique triangles   : " << numUniqueTris << endl;
    cout << "  - num instanced triangles: " << numInstancedTris << endl;

    if (numInstancedTris == 0) 
      error("no (valid) input files specified - model contains no triangles");

    if (msgModel->material.empty()) {
      cout << "msgView: adding default material" << endl;
      msgModel->material.push_back(new miniSG::Material);
    }

    // -------------------------------------------------------
    // create ospray model
    // -------------------------------------------------------
    ospModel = ospNewModel();

    // code does not yet do instancing ... check that the model doesn't contain instances
    for (int i=0;i<msgModel->instance.size();i++)
      if (msgModel->instance[i] != miniSG::Instance(i))
        error("found a scene that seems to contain instances, "
              "but msgView does not yet support instancing");
    
    cout << "msgView: adding parsed geometries to ospray model" << endl;
    for (int i=0;i<msgModel->mesh.size();i++) {
      Ref<miniSG::Mesh> msgMesh = msgModel->mesh[i];

      // create ospray mesh
      OSPGeometry ospMesh = ospNewTriangleMesh();
      
      // add position array to mesh
      OSPData position = ospNewData(msgMesh->position.size(),OSP_vec3fa,
                                    &msgMesh->position[0]);
      ospSetData(ospMesh,"position",position);
      
      // add triangle index array to mesh
      OSPData index = ospNewData(msgMesh->triangle.size(),OSP_vec4i,
                                 &msgMesh->triangle[0]);
      ospSetData(ospMesh,"index",index);
      
      ospAddGeometry(ospModel,ospMesh);
    }
    ospCommit(ospModel);
    cout << "msgView: done creating ospray model." << endl;

    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    MSGViewer window(ospModel);
    window.create("MSGViewer: OSPRay Mini-Scene Graph test viewer");
    printf("MSG Viewer created. Press 'Q' to quit.\n");
    ospray::glut3D::runGLUT();
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::stlViewerMain(ac,av);
}
