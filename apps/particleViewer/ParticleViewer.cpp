// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

/*! \file apps/particleViewer/viewer.cpp 
  \brief A GLUT-based viewer for simple particle data */

// viewer widget
#include "apps/common/widgets/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
// particle viewer
#include "Model.h"
#include "uintah.h"
// embree
#include "sys/filename.h"

namespace ospray {
  namespace particle {

    using std::cout;
    using std::endl;

    int maxAccum = 64;
    int accumID = 0;
    int timeStep = 0;
    std::vector<OSPModel> modelTimeStep;

    std::string modelSaveFileName = "";
    OSPRenderer        ospRenderer = NULL;
    typedef enum { LAMMPS_XYZ, DAT_XYZ } InputFormat;

    /*! when using the OBJ renderer, we create a automatic dirlight with this direction; use ''--sun-dir x y z' to change */
    //  vec3f defaultDirLight_direction(-.3, -1, .4);
    vec3f defaultDirLight_direction(.3, -1, -.2);

    //! the renderer we're about to use
    std::string rendererType = "ao1";
    //    std::string rendererType = "raycast_eyelight";
    float radius = 1.f;
    InputFormat inputFormat = LAMMPS_XYZ;
    void error(const std::string &err)
    {
      cout << "#osp::ospParticleViewer fatal error : " << err << endl;
      cout << endl;
      cout << "Proper usage: " << endl;
      cout << "  ./ospParticleViewer <inFileName[.xyz|.xml]>" << endl;
      cout << endl;
      exit(1);
    }

    using ospray::glut3D::Glut3DWidget;
    
    /*! mini scene graph viewer widget. \internal Note that all handling
      of camera is almost exactly similar to the code in volView;
      might make sense to move that into a common class! */
    struct ParticleViewer : public Glut3DWidget {
      ParticleViewer(OSPModel model, OSPRenderer renderer)
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
          fb(NULL), renderer(renderer), model(model)
      {
        Assert(model && "null model handle");
        camera = ospNewCamera("perspective");
        Assert(camera != NULL && "could not create camera");
        ospSet3f(camera,"pos",-1,1,-1);
        ospSet3f(camera,"dir",+1,-1,+1);
        ospCommit(camera);

        ospSetObject(renderer,"model",model);
        ospSetObject(renderer,"camera",camera);
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
        case 'Q': exit(0);
        case '>': {
          timeStep = (timeStep+1)%modelTimeStep.size();
          model = modelTimeStep[timeStep];
          ospSetObject(renderer,"model",model);
          PRINT(timeStep);
          PRINT(model);
          ospCommit(renderer);
          viewPort.modified = true;
          forceRedraw();
        } break;
        case '<': {
          timeStep = (timeStep+modelTimeStep.size()-1)%modelTimeStep.size();
          model = modelTimeStep[timeStep];
          PRINT(timeStep);
          PRINT(model);
          ospSetObject(renderer,"model",model);
          ospCommit(renderer);
          viewPort.modified = true;
          forceRedraw();
        } break;
        default:
          Glut3DWidget::keypress(key,where);
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

        sprintf(title,"OSPRay Particle Viewer");
        setTitle(title);
        // }
      }
    
      OSPModel       model;
      OSPFrameBuffer fb;
      OSPRenderer    renderer;
      OSPCamera      camera;
      ospray::glut3D::FPSCounter fps;
    };

    particle::Model *createTestCube(int numPerSide)
    {
      particle::Model *m = new particle::Model;
      int type = m->getAtomType("testParticle");
      for (int z=0;z<numPerSide;z++)
        for (int y=0;y<numPerSide;y++)
          for (int x=0;x<numPerSide;x++) {
            particle::Model::Atom a;
            a.position.x = x/float(numPerSide);
            a.position.y = y/float(numPerSide);
            a.position.z = z/float(numPerSide);
            a.type = type;
            m->atom.push_back(a);
          }
      m->radius = 1.f/numPerSide;
      return m;
    }

    OSPData makeMaterials(OSPRenderer renderer,particle::Model *model)
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

    void ospParticleViewerMain(int &ac, const char **&av)
    {
      std::vector<Model *> particleModel;
    
      cout << "ospParticleViewer: starting to process cmdline arguments" << endl;
      std::vector<std::pair<particle::Model *, std::string> > deferredLoadingListXYZ;

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "--renderer") {
          assert(i+1 < ac);
          rendererType = av[++i];
        } else if (arg == "--radius") {
          radius = atof(av[++i]);
          PRINT(radius);
        } else if (arg == "--sun-dir") {
          defaultDirLight_direction.x = atof(av[++i]);
          defaultDirLight_direction.y = atof(av[++i]);
          defaultDirLight_direction.z = atof(av[++i]);
        } else if (arg == "--module" || arg == "--plugin") {
          assert(i+1 < ac);
          const char *moduleName = av[++i];
          cout << "loading ospray module '" << moduleName << "'" << endl;
          ospLoadModule(moduleName);
        } else if (arg == "--save-to") {
          modelSaveFileName = av[++i];
        } else if (av[i][0] == '-') {
          error("unkown commandline argument '"+arg+"'");
        } else {
          embree::FileName fn = arg;
          if (fn.str() == "___CUBE_TEST___") {
            int numPerSide = atoi(av[++i]);
            particle::Model *m = createTestCube(numPerSide);
            particleModel.push_back(m);
          } else if (fn.ext() == "xyz") {
            particle::Model *m = new particle::Model;
            //            m->loadXYZ(fn);
            std::pair<particle::Model *, embree::FileName> loadJob(m,fn.str());
            deferredLoadingListXYZ.push_back(loadJob);
            particleModel.push_back(m);
          } else if (fn.ext() == "xyz2") {
            particle::Model *m = new particle::Model;
            m->loadXYZ2(fn);
            particleModel.push_back(m);
          } else if (fn.ext() == "xml") {
            particle::Model *m = parse__Uintah_timestep_xml(fn);
            particleModel.push_back(m);
          } else
            error("unknown file format "+fn.str());
        }
      }
      if (particleModel.empty())
        error("no input file specified");

#pragma omp parallel 
      {
#pragma omp for
        for (int i=0;i<deferredLoadingListXYZ.size();i++) {
          deferredLoadingListXYZ[i].first->loadXYZ(deferredLoadingListXYZ[i].second);
        }
      }


      // -------------------------------------------------------
      // done parsings
      // -------------------------------------------------------]
      cout << "ospParticleViewer: done parsing. found model with" << endl;
      cout << "  - num atoms: " << particleModel[0]->atom.size() << endl;
      if (modelSaveFileName != "") {
        particleModel[0]->saveToFile(modelSaveFileName);
        exit(0);
      }

      // -------------------------------------------------------
      // create ospray model
      // -------------------------------------------------------
      ospRenderer = ospNewRenderer(rendererType.c_str());
      if (!ospRenderer)
        error("could not create ospRenderer '"+rendererType+"'");
      Assert(ospRenderer != NULL && "could not create ospRenderer");

      for (int i=0;i<particleModel.size();i++) {
        OSPModel model = ospNewModel();
        OSPData materialData = makeMaterials(ospRenderer,particleModel[i]);
    
        OSPData data = ospNewData(particleModel[i]->atom.size()*4,OSP_FLOAT,
                                  &particleModel[i]->atom[0],OSP_DATA_SHARED_BUFFER);
        ospCommit(data);

        OSPGeometry geom = ospNewGeometry("spheres");
        ospSet1f(geom,"radius",radius*particleModel[i]->radius);
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


      cout << "ospParticleViewer: done creating ospray model." << endl;

      // -------------------------------------------------------
      // create viewer window
      // -------------------------------------------------------
      ParticleViewer window(model,ospRenderer);
      window.create("ospray particle viewer");
      printf("OSPRay Particle Viewer created. Press 'Q' to quit.\n");
      box3f wb(particleModel[0]->getBBox());
      PRINT(wb);
      window.setWorldBounds(wb);
      ospray::glut3D::runGLUT();
    }

  } // ::ospray::particle
} // ::ospray


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::particle::ospParticleViewerMain(ac,av);
}
