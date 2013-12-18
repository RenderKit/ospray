/*! \file objView.cpp A GLUT-based viewer for Wavefront OBJ files */

// viewer widget
#include "../util/glut3D/glut3D.h"
#include "../util/miniSG/miniSG.h"
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;

  Ref<miniSG::Model> model = NULL;

  void error(const std::string &msg)
  {
    cout << "ospray::stlViewer fatal error : " << msg << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./stlViewer <inFileName>" << endl;
    cout << endl;
    exit(1);
  }

  void stlViewerMain(int &ac, const char **&av)
  {
    model = new miniSG::Model;

    // -------------------------------------------------------
    // parse cmdline
    // -------------------------------------------------------
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (av[i][0] == '-') {

        error("unkown commandline argument '"+arg+"'");
      } else {
        embree::FileName fn = arg;
        if (fn.ext() == "stl")
          miniSG::importSTL(*model,fn);
        else if (fn.ext() == "msg")
          miniSG::importMSG(*model,fn);
        else if (fn.ext() == "obj")
          miniSG::importOBJ(*model,fn);
        else
          error("unrecognized file format in filename '"+arg+"'");
      }
    }
    // -------------------------------------------------------
    // done parsing
    // -------------------------------------------------------
    cout << "done parsing. found model with" << endl;
    cout << "  - num materials: " << model->material.size() << endl;
    cout << "  - num meshes: " << model->mesh.size() << " ";
    int numUniqueTris = 0;
    int numInstancedTris = 0;
    for (int i=0;i<model->mesh.size();i++) {
      cout << "[" << model->mesh[i]->size() << "]";
      numUniqueTris += model->mesh[i]->size();
    }
    cout << endl;
    cout << "  - num instances: " << model->instance.size() << " ";
    for (int i=0;i<model->mesh.size();i++) {
      cout << "[" << model->mesh[model->instance[i].meshID]->size() << "]";
      numInstancedTris += model->mesh[model->instance[i].meshID]->size();
    }
    cout << endl;
    cout << "  - num unique triangles   : " << numUniqueTris << endl;
    cout << "  - num instanced triangles: " << numInstancedTris << endl;

    if (model->material.empty()) {
      cout << "adding default material" << endl;
      model->material.push_back(new miniSG::Material);
    }

    // -------------------------------------------------------
    // create ospray model
    // -------------------------------------------------------
  }
}


int main(int ac, const char **av)
{
  ospray::stlViewerMain(ac,av);
}
