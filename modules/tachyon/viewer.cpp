/*! \file tachView.cpp A GLUT-based viewer for Tachyon files */

#undef NDEBUG
// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
#include "model.h"

namespace ospray {
  namespace tachyon {
    using std::cout;
    using std::endl;

    const char *renderType = "raycast_eyelight";
    // const char *renderType = "ao16";
    float defaultRadius = .1f;
    tachyon::Model tachModel;
  
    void error(const std::string &vol)
    {
      if (vol != "") {
        cout << "=======================================================" << endl;
        cout << "ospray::tachView fatal error : " << vol << endl;
        cout << "=======================================================" << endl;
      }
      cout << endl;
      cout << "Proper usage: " << endl;
      cout << "  ./ospTachyon <inputfile.tach> <options>" << std::endl;
      cout << "options:" << endl;
      cout << " <none>" << endl;
      cout << endl;
      exit(1);
    }

    OSPModel specifyModel(tachyon::Model &tm)
    {
      OSPModel ospModel = ospNewModel();
    
      if (tm.numSpheres()) {
        OSPData  sphereData = ospNewData(tm.numSpheres()*sizeof(Sphere)/sizeof(float),
                                         OSP_FLOAT,tm.getSpherePtr());
        ospCommit(sphereData);
        OSPGeometry sphereGeom = ospNewGeometry("spheres");
        ospSetData(sphereGeom,"spheres",sphereData);
        ospSet1i(sphereGeom,"bytes_per_sphere",sizeof(Sphere));
        ospSet1i(sphereGeom,"offset_materialID",0*sizeof(float));
        ospSet1i(sphereGeom,"offset_center",1*sizeof(float));
        ospSet1i(sphereGeom,"offset_radius",4*sizeof(float));
        ospCommit(sphereGeom);
        ospAddGeometry(ospModel,sphereGeom);
      }

      if (tm.numCylinders()) {
        OSPData  cylinderData = ospNewData(tm.numCylinders()*sizeof(Cylinder)/sizeof(float),
                                           OSP_FLOAT,tm.getCylinderPtr());
        ospCommit(cylinderData);
        OSPGeometry cylinderGeom = ospNewGeometry("cylinders");
        ospSetData(cylinderGeom,"cylinders",cylinderData);
        ospSet1i(cylinderGeom,"bytes_per_cylinder",sizeof(Cylinder));
        ospSet1i(cylinderGeom,"offset_materialID",0*sizeof(float));
        ospSet1i(cylinderGeom,"offset_v0",1*sizeof(float));
        ospSet1i(cylinderGeom,"offset_v1",4*sizeof(float));
        ospSet1i(cylinderGeom,"offset_radius",7*sizeof(float));
        ospCommit(cylinderGeom);
        ospAddGeometry(ospModel,cylinderGeom);
      }

      cout << "#osp:tach: creating " << tm.numVertexArrays() << " vertex arrays" << endl;
      for (int vaID=0;vaID<tm.numVertexArrays();vaID++) {
        const VertexArray *va = tm.getVertexArray(vaID);
        Assert(va);
        OSPGeometry geom = ospNewTriangleMesh();
        if (va->triangle.size()) {
          OSPData data = ospNewData(va->triangle.size(),OSP_vec3i,&va->triangle[0]);
          ospCommit(data);
          ospSetData(geom,"triangle",data);
        }
        if (va->coord.size()) {
          OSPData data = ospNewData(va->coord.size(),OSP_vec3fa,&va->coord[0]);
          ospCommit(data);
          ospSetData(geom,"vertex",data);
        }
        if (va->normal.size()) {
          OSPData data = ospNewData(va->normal.size(),OSP_vec3fa,&va->normal[0]);
          ospCommit(data);
          ospSetData(geom,"vertex_normal",data);
        }
        if (va->color.size()) {
          OSPData data = ospNewData(va->color.size(),OSP_vec3fa,&va->color[0]);
          ospCommit(data);
          ospSetData(geom,"vertex_color",data);
        }
        ospCommit(geom);
        ospAddGeometry(ospModel,geom);
      }


// #if 0
//       if (tm.numTriangles()) {
//         OSPData  triangleData = ospNewData(tm.numTriangles()*sizeof(Triangle)/sizeof(float),OSP_FLOAT,
//                                          tm.getTrianglePtr());
//         OSPGeometry triangleGeom = ospNewGeometry("tachyon_triangles");
//         ospSetData(triangleGeom,"triangles",triangleData);
//         ospCommit(triangleGeom);
//         ospAddGeometry(ospModel,triangleGeom);
//       }

//       if (tm.numCylinders()) {
//         OSPData  cylinderData = ospNewData(tm.numCylinders()*sizeof(Cylinder)/sizeof(float),OSP_FLOAT,
//                                          tm.getCylinderPtr());
//         OSPGeometry cylinderGeom = ospNewGeometry("tachyon_cylinders");
//         ospSetData(cylinderGeom,"cylinders",cylinderData);
//         ospCommit(cylinderGeom);
//         ospAddGeometry(ospModel,cylinderGeom);
//       }
// #endif
      ospCommit(ospModel);
      return ospModel;
    }

    using ospray::glut3D::Glut3DWidget;

    //! volume viewer widget. 
    /*! \internal Note that all handling of camera is almost exactly
      similar to the code in msgView; might make sense to move that into
      a common class! */
    struct TACHViewer : public Glut3DWidget {
      /*! construct volume from file name and dimensions \see volview_notes_on_volume_interface */
      TACHViewer(OSPModel model) 
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
          fb(NULL), renderer(NULL), model(model)
      {
        camera = ospNewCamera("perspective");
        Assert2(camera,"could not create camera");
        ospSet3f(camera,"pos",-1,1,-1);
        ospSet3f(camera,"dir",+1,-1,+1);
        ospCommit(camera);
        ospCommit(camera);

        renderer = ospNewRenderer(renderType);

        Assert2(renderer,"could not create renderer");
        ospSetParam(renderer,"model",model);
        ospSetParam(renderer,"camera",camera);
        ospCommit(renderer);

      };
      virtual void reshape(const ospray::vec2i &newSize) 
      {
        Glut3DWidget::reshape(newSize);
        if (fb) ospFreeFrameBuffer(fb);
        fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
      }

      virtual void display() 
      {
        if (!fb || !renderer) return;

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
    
        forceRedraw();
      }

      OSPModel       model;
      OSPFrameBuffer fb;
      OSPRenderer    renderer;
      OSPCamera      camera;
      ospray::glut3D::FPSCounter fps;
    };

    void ospTACHMain(int &ac, const char **&av)
    {
      ospLoadModule("tach");

    
      for (int i=1;i<ac;i++) {
        std::string arg = av[i];
        if (arg[0] != '-') {
          importFile(tachModel,arg);
        } else 
          throw std::runtime_error("unknown parameter "+arg);
      }

      // -------------------------------------------------------
      // parse and set up input(s)
      // -------------------------------------------------------
      if (tachModel.empty())
        error("no input geometry specifies!?");
      OSPModel model = specifyModel(tachModel);
      // -------------------------------------------------------
      // create viewer window
      // -------------------------------------------------------
      TACHViewer window(model);
      window.create("ospTACH: OSPRay Tachyon-model viewer");
      printf("Viewer created. Press 'Q' to quit.\n");
      window.setWorldBounds(tachModel.getBounds());
      if (tachModel.getCamera()) {
        ospray::tachyon::Camera *camera = tachModel.getCamera();
        window.viewPort.from = camera->center;
        window.viewPort.at = camera->center+camera->viewDir;
        window.viewPort.up = camera->upDir;
        window.computeFrame();
      }
      ospray::glut3D::runGLUT();
    }
  }
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::tachyon::ospTACHMain(ac,av);
}
