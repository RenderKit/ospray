#undef NDEBUG

// viewer widget
#include "apps/common/widgets/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
// tachyon module
#include "Model.h"

namespace ospray {
  namespace tachyon {
    using std::cout;
    using std::endl;

    float lightScale = 1.f;
    const char *renderType = "tachyon";
    // const char *renderType = "ao16";
    float defaultRadius = .1f;

    struct TimeStep {
      std::string modelName;
      tachyon::Model tm; // tachyon model
      OSPModel       om; // ospray model

      TimeStep(const std::string &modelName) : modelName(modelName), om(NULL) {};
    };

    std::vector<TimeStep *> timeStep;
    int timeStepID = 0;
    bool doShadows = true;

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
                                         OSP_FLOAT,tm.getSpheresPtr());
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
                                           OSP_FLOAT,tm.getCylindersPtr());
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

      cout << "#osp:tachyon: creating " << tm.numVertexArrays() << " vertex arrays" << endl;
      long numTriangles = 0;
      for (int vaID=0;vaID<tm.numVertexArrays();vaID++) {
        const VertexArray *va = tm.getVertexArray(vaID);
        // if (va != tm.getSTriVA())
        //   continue;
        Assert(va);
        OSPGeometry geom = ospNewTriangleMesh();
        numTriangles += va->triangle.size();
        if (va->triangle.size()) {
          OSPData data = ospNewData(va->triangle.size(),OSP_INT3,&va->triangle[0]);
          ospCommit(data);
          ospSetData(geom,"triangle",data);
        }
        if (va->coord.size()) {
          OSPData data = ospNewData(va->coord.size(),OSP_FLOAT3A,&va->coord[0]);
          ospCommit(data);
          ospSetData(geom,"vertex",data);
        }
        if (va->normal.size()) {
          OSPData data = ospNewData(va->normal.size(),OSP_FLOAT3A,&va->normal[0]);
          ospCommit(data);
          ospSetData(geom,"vertex.normal",data);
        }
        if (va->color.size()) {
          OSPData data = ospNewData(va->color.size(),OSP_FLOAT3A,&va->color[0]);
          ospCommit(data);
          ospSetData(geom,"vertex.color",data);
        }
        if (va->perTriTextureID.size()) {
          OSPData data = ospNewData(va->perTriTextureID.size(),OSP_UINT,
                                    &va->perTriTextureID[0]);
          ospCommit(data);
          ospSetData(geom,"prim.materialID",data);
        } else {
          ospSet1i(geom,"geom.materialID",va->textureID);
        }
        ospCommit(geom);
        ospAddGeometry(ospModel,geom);
      }


      cout << "#osp:tachyon: specifying " << tm.numTextures() << " materials..." << endl;
      {
        OSPData data = ospNewData(tm.numTextures()*sizeof(Texture),
                                  OSP_UCHAR,tm.getTexturesPtr());
        ospCommit(data);
        ospSetData(ospModel,"textureArray",data);
      }

      cout << "#osp:tachyon: specifying " << tm.numPointLights()
           << " point lights..." << endl;
      if (tm.numPointLights() > 0)
      {
        OSPData data
          = ospNewData(tm.numPointLights()*sizeof(PointLight),
                       OSP_UCHAR,tm.getPointLightsPtr());
        ospCommit(data);
        ospSetData(ospModel,"pointLights",data);
      }

      cout << "#osp:tachyon: specifying " << tm.numDirLights()
           << " dir lights..." << endl;
      if (tm.numDirLights() > 0)
      {
        OSPData data
          = ospNewData(tm.numDirLights()*sizeof(DirLight),
                       OSP_UCHAR,tm.getDirLightsPtr());
        ospCommit(data);
        ospSetData(ospModel,"dirLights",data);
      }

      std::cout << "=======================================" << std::endl;
      std::cout << "Tachyon Renderer: Done specifying model" << std::endl;
      std::cout << "num spheres: " << tm.numSpheres() << std::endl;
      std::cout << "num cylinders: " << tm.numCylinders() << std::endl;
      std::cout << "num triangles: " << numTriangles << std::endl;
      std::cout << "=======================================" << std::endl;


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
      TACHViewer(OSPModel model, const std::string &modelName) 
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
          fb(NULL), renderer(NULL), model(model), modelName(modelName)
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
        ospSet1i(renderer,"do_shadows",doShadows);
        ospCommit(renderer);

      };

      virtual void keypress(char key, const vec2f where)
      {
        switch (key) {
        case 'X':
          if (viewPort.up == vec3f(1,0,0) || viewPort.up == vec3f(-1.f,0,0))
            viewPort.up = - viewPort.up;
          else 
            viewPort.up = vec3f(1,0,0);
          viewPort.modified = true;
          break;
        case 'Y':
          if (viewPort.up == vec3f(0,1,0) || viewPort.up == vec3f(0,-1.f,0))
            viewPort.up = - viewPort.up;
          else 
            viewPort.up = vec3f(0,1,0);
          viewPort.modified = true;
          break;
        case 'Z':
          if (viewPort.up == vec3f(0,0,1) || viewPort.up == vec3f(0,0,-1.f))
            viewPort.up = - viewPort.up;
          else 
            viewPort.up = vec3f(0,0,1);
          viewPort.modified = true;
          break;
        case 'S':
          doShadows = !doShadows;
          cout << "Switching shadows " << (doShadows?"ON":"OFF") << endl;
          ospSet1i(renderer,"do_shadows",doShadows);
          ospCommit(renderer);
          break;
        case 'L':
          lightScale *= 1.5f;
          ospSet1f(renderer,"lightScale",lightScale);
          PRINT(lightScale);
          PRINT(renderer);
          ospCommit(renderer);
          break;
        case 'l':
          lightScale /= 1.5f;
          PRINT(lightScale);
          PRINT(renderer);
          ospSet1f(renderer,"lightScale",lightScale);
          ospCommit(renderer);
          break;
        case '<':
          setTimeStep((timeStepID+timeStep.size()-1)%timeStep.size());
          break;
        case '>':
          setTimeStep((timeStepID+1)%timeStep.size());
          break;
        default:
          Glut3DWidget::keypress(key,where);
        }
      }

      void setTimeStep(size_t newTSID)
      {
        timeStepID = newTSID;
        modelName = timeStep[timeStepID]->modelName;
        cout << "#osp:tachyon: switching to time step " << timeStepID
             << " (" << modelName << ")" << endl;
        model = timeStep[timeStepID]->om;
        ospSetParam(renderer,"model",model);
        ospCommit(renderer);
      }


      virtual void reshape(const ospray::vec2i &newSize) 
      {
        Glut3DWidget::reshape(newSize);
        if (fb) ospFreeFrameBuffer(fb);
        fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8,OSP_FB_COLOR|OSP_FB_ACCUM);
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
          ospFrameBufferClear(fb,OSP_FB_COLOR|OSP_FB_ACCUM);
        }

        fps.startRender();
        ospRenderFrame(fb,renderer,OSP_FB_COLOR|OSP_FB_ACCUM);
        fps.doneRender();
    
        ucharFB = (unsigned int *)ospMapFrameBuffer(fb);
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        Glut3DWidget::display();
    
        ospUnmapFrameBuffer(ucharFB,fb);
    
#if 1
        char title[10000];
        sprintf(title,"ospray Tachyon viewer (%s) [%f fps]",
               modelName.c_str(),fps.getFPS());
        setTitle(title);
#endif

        forceRedraw();
      }

      OSPModel       model;
      OSPFrameBuffer fb;
      OSPRenderer    renderer;
      OSPCamera      camera;
      ospray::glut3D::FPSCounter fps;
      std::string modelName;
    };

    void ospTACHMain(int &ac, const char **&av)
    {
      ospLoadModule("tachyon");

      for (int i=1;i<ac;i++) {
        std::string arg = av[i];
        if (arg[0] != '-') {                    
          timeStep.push_back(new TimeStep(arg));
        } else 
          throw std::runtime_error("unknown parameter "+arg);
      }

      if (timeStep.empty())
        error("no input geometry specifies!?");

      std::vector<OSPModel> modelHandle;
      for (int i=0;i<timeStep.size();i++) {
        TimeStep *ts = timeStep[i];
        importFile(ts->tm,ts->modelName);
        if (ts->tm.empty())
          error(ts->modelName+": no input geometry specified!?");
        ts->om = specifyModel(ts->tm);
      }


      // -------------------------------------------------------
      // parse and set up input(s)
      // -------------------------------------------------------

      // -------------------------------------------------------
      // create viewer window
      // -------------------------------------------------------
      TACHViewer window(timeStep[0]->om,timeStep[0]->modelName);
      window.create("ospTACH: OSPRay Tachyon-model viewer");
      printf("Viewer created. Press 'Q' to quit.\n");
      window.setWorldBounds(timeStep[0]->tm.getBounds());
      ospray::tachyon::Camera *camera = timeStep[0]->tm.getCamera();
      if (camera) {
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
