/*! \file lammpsView.cpp A GLUT-based viewer for simple geometry
  (supports STL and Wavefront OBJ files) */

// viewer widget
#include "glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
#include "lammpsModel.h"
#include "sys/filename.h"

namespace ospray {
  namespace lammps {
    using std::cout;
    using std::endl;

    int maxAccum = 64;
    int accumID = 0;
    int timeStep = 0;
    std::vector<OSPModel> modelTimeStep;

    OSPRenderer        ospRenderer = NULL;
    typedef enum { LAMMPS_XYZ, DAT_XYZ } InputFormat;

    /*! when using the OBJ renderer, we create a automatic dirlight with this direction; use ''--sun-dir x y z' to change */
    //  vec3f defaultDirLight_direction(-.3, -1, .4);
    vec3f defaultDirLight_direction(.3, -1, -.2);

    //! the renderer we're about to use
    std::string rendererType = "raycast_eyelight";
    float radius = 30.f;
    InputFormat inputFormat = LAMMPS_XYZ;
    void error(const std::string &lammps)
    {
      cout << "ospray::lammpsView fatal error : " << lammps << endl;
      cout << endl;
      cout << "Proper usage: " << endl;
      cout << "  ./lammpsView <inFileName.xyz>" << endl;
      cout << endl;
      exit(1);
    }

    using ospray::glut3D::Glut3DWidget;
    
    /*! mini scene graph viewer widget. \internal Note that all handling
      of camera is almost exactly similar to the code in volView;
      might make sense to move that into a common class! */
    struct LAMMPSViewer : public Glut3DWidget {
      LAMMPSViewer(OSPModel model, OSPRenderer renderer)
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
          fb(NULL), renderer(renderer), model(model)
      {
        Assert(model && "null model handle");
        camera = ospNewCamera("perspective");
        Assert(camera != NULL && "could not create camera");
        ospSet3f(camera,"pos",-1,1,-1);
        ospSet3f(camera,"dir",+1,-1,+1);
        ospCommit(camera);

        ospSetParam(renderer,"model",model);
        ospSetParam(renderer,"camera",camera);
        ospCommit(camera);
        ospCommit(renderer);
      };

      virtual void reshape(const ospray::vec2i &newSize)
      {
        Glut3DWidget::reshape(newSize);
        if (fb) ospFreeFrameBuffer(fb);
        fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8,OSP_FB_COLOR|OSP_FB_ACCUM);
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        accumID = 0;
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
      }

      virtual void keypress(char key, const vec2f where)
      {
        switch(key) {
        case '>': {
          timeStep = (timeStep+1)%modelTimeStep.size();
          model = modelTimeStep[timeStep];
          ospSetParam(renderer,"model",model);
          PRINT(timeStep);
          PRINT(model);
          ospCommit(renderer);
          viewPort.modified = true;
        } break;
        case '<': {
          timeStep = (timeStep+modelTimeStep.size()-1)%modelTimeStep.size();
          model = modelTimeStep[timeStep];
          PRINT(timeStep);
          PRINT(model);
          ospSetParam(renderer,"model",model);
          ospCommit(renderer);
          viewPort.modified = true;
        } break;
        }
      }

      virtual void display()
      {
        if (!fb || !renderer) return;
      
        static int frameID = 0;
      
        //{
        // note that the order of 'start' and 'end' here is
        // (intentionally) reversed: due to our asynchrounous rendering
        // you cannot place start() and end() _around_ the renderframe
        // call (which in itself will not do a lot other than triggering
        // work), but the average time between ttwo calls is roughly the
        // frame rate (including display overhead, of course)
        if (frameID > 0) fps.doneRender();
        fps.startRender();
        //}
        ++frameID;
      
        if (viewPort.modified) {
          Assert2(camera,"ospray camera is null");
          ospSetVec3f(camera,"pos",viewPort.from);
          ospSetVec3f(camera,"dir",viewPort.at-viewPort.from);
          ospSetVec3f(camera,"up",viewPort.up);
          ospSetf(camera,"aspect",viewPort.aspect);
          ospCommit(camera);
          ospFrameBufferClear(fb,OSP_FB_ACCUM);
          viewPort.modified = false;
          accumID = 0;
        }
      
        ospRenderFrame(fb,renderer,OSP_FB_COLOR|OSP_FB_ACCUM);
        ++accumID;
        if (accumID < maxAccum) 
          forceRedraw();

        ucharFB = (uint32 *) ospMapFrameBuffer(fb);
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        Glut3DWidget::display();
      
        ospUnmapFrameBuffer(ucharFB,fb);
      
        char title[1000];

        // if (alwaysRedraw) {
        //   sprintf(title,"OSPRay LAMMPSView (%f fps)",
        //           fps.getFPS());
        //   setTitle(title);
        //   forceRedraw();
        // } else {
        sprintf(title,"OSPRay Lammps Viewer");
        setTitle(title);
        // }
      }
    
      OSPModel       model;
      OSPFrameBuffer fb;
      OSPRenderer    renderer;
      OSPCamera      camera;
      ospray::glut3D::FPSCounter fps;
    };

    OSPData makeMaterials(OSPRenderer renderer,Model *model)
    {
      int numMaterials = model->atomType.size();
      OSPMaterial matArray[numMaterials];
      for (int i=0;i<numMaterials;i++) {
        OSPMaterial mat = ospNewMaterial(renderer,"OBJMaterial");
        assert(mat);
        ospSet3fv(mat,"kd",&model->atomType[i]->color.x);
        ospCommit(mat);
        matArray[i] = mat;
      }
      OSPData data = ospNewData(numMaterials,OSP_OBJECT,matArray);
      ospCommit(data);
      return data;
    }

    void lammpsViewMain(int &ac, const char **&av)
    {
      std::vector<Model *> lammpsModel;
    
      cout << "lammpsView: starting to process cmdline arguments" << endl;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "--renderer") {
          assert(i+1 < ac);
          rendererType = av[++i];
        } else if (arg == "--radius") {
          radius = atof(av[++i]);
          PRINT(radius);
        } else if (arg == "--dat-xyz") {
          inputFormat = DAT_XYZ;
        } else if (arg == "--lammps") {
          inputFormat = LAMMPS_XYZ;
        } else if (arg == "--sun-dir") {
          defaultDirLight_direction.x = atof(av[++i]);
          defaultDirLight_direction.y = atof(av[++i]);
          defaultDirLight_direction.z = atof(av[++i]);
        } else if (arg == "--module" || arg == "--plugin") {
          assert(i+1 < ac);
          const char *moduleName = av[++i];
          cout << "loading ospray module '" << moduleName << "'" << endl;
          ospLoadModule(moduleName);
        } else if (av[i][0] == '-') {
          error("unkown commandline argument '"+arg+"'");
        } else {
          embree::FileName fn = arg;
          if (fn.ext() == "xyz" || fn.ext() == "dat") {
            lammps::Model *m = new lammps::Model;
            if (inputFormat == LAMMPS_XYZ) 
              m->load_LAMMPS_XYZ(fn.str());
            else 
              m->load_DAT_XYZ(fn.str());
            lammpsModel.push_back(m);
          } else
            error("unknown file format "+fn.str());
        }
      }
      if (lammpsModel.empty())
        error("no input file specified");

      // -------------------------------------------------------
      // done parsings
      // -------------------------------------------------------]
      cout << "lammpsView: done parsing. found model with" << endl;
      cout << "  - num atoms: " << lammpsModel[0]->atom.size() << endl;

      // -------------------------------------------------------
      // create ospray model
      // -------------------------------------------------------
      ospRenderer = ospNewRenderer(rendererType.c_str());
      if (!ospRenderer)
        error("could not create ospRenderer '"+rendererType+"'");
      Assert(ospRenderer != NULL && "could not create ospRenderer");

      for (int i=0;i<lammpsModel.size();i++) {
        OSPModel model = ospNewModel();
        OSPData materialData = makeMaterials(ospRenderer,lammpsModel[i]);
    
        OSPData data = ospNewData(lammpsModel[i]->atom.size()*4,OSP_FLOAT,
                                  &lammpsModel[i]->atom[0],OSP_DATA_SHARED_BUFFER);
        ospCommit(data);

        OSPGeometry geom = ospNewGeometry("spheres");
        ospSet1f(geom,"radius",radius);
        ospSet1i(geom,"bytes_per_sphere",sizeof(Model::Atom));
        ospSet1i(geom,"center_offset",0);
        ospSet1i(geom,"offset_materialID",3*sizeof(float));
        ospSetData(geom,"spheres",data);
        ospSetData(geom,"materialList",materialData);
        ospCommit(geom);

        ospAddGeometry(model,geom);
        ospCommit(model);

        modelTimeStep.push_back(model);
      }

      OSPModel model = modelTimeStep[timeStep];

      std::vector<OSPLight> dirLights;
      cout << "msgView: Adding a hard coded directional light as the sun." << endl;
      OSPLight ospLight = ospNewLight(ospRenderer, "DirectionalLight");
      ospSetString(ospLight, "name", "sun" );
      ospSet3f(ospLight, "color", 1, 1, 1);
      ospSet3fv(ospLight, "direction", &defaultDirLight_direction.x);
      ospCommit(ospLight);
      dirLights.push_back(ospLight);
      OSPData dirLightArray = ospNewData(dirLights.size(), OSP_OBJECT, &dirLights[0], 0);
      ospSetData(ospRenderer, "directionalLights", dirLightArray);

      ospCommit(ospRenderer);


      cout << "lammpsView: done creating ospray model." << endl;

      // -------------------------------------------------------
      // create viewer window
      // -------------------------------------------------------
      LAMMPSViewer window(model,ospRenderer);
      window.create("LAMMPSViewer: OSPRay Mini-Scene Graph test viewer");
      printf("LAMMPS Viewer created. Press 'Q' to quit.\n");
      window.setWorldBounds(box3f(lammpsModel[0]->getBBox()));
      ospray::glut3D::runGLUT();
    }
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::lammps::lammpsViewMain(ac,av);
}
