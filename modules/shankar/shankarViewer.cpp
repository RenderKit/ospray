
/*! \file shankarView.cpp A GLUT-based viewer for simple geometry
  (supports STL and Wavefront OBJ files) */

// viewer widget
#include "glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
#include "sys/filename.h"
#include "shankarModel.h"
#include "rbfTree.h"

namespace ospray {
  namespace shankar {
    using std::cout;
    using std::endl;

    OSPRenderer        ospRenderer = NULL;
    box3f rbfBounds;
    bool alwaysRedraw = 1;

    //! the renderer we're about to use
    std::string rendererType = "primID";
    float radius = 3.f;

    void error(const std::string &shankar)
    {
      cout << "ospray::shankarView fatal error : " << shankar << endl;
      cout << endl;
      cout << "Proper usage: " << endl;
      cout << "  ./shankarView <inputfile.tree>" << endl;
      cout << endl;
      exit(1);
    }

    using ospray::glut3D::Glut3DWidget;
  
    /*! mini scene graph viewer widget. \internal Note that all handling
      of camera is almost exactly similar to the code in volView;
      might make sense to move that into a common class! */
    struct ShankarViewer : public Glut3DWidget {
      ShankarViewer(OSPModel model, OSPRenderer renderer)
        : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
          fb(NULL), renderer(renderer), model(model)
      {
        Assert(model && "null model handle");
        camera = ospNewCamera("perspective");
        Assert(camera != NULL && "could not create camera");
        ospSet3f(camera,"pos",-1,1,-1);
        ospSet3f(camera,"dir",+1,-1,+1);
        ospCommit(camera);

        PRINT(renderer);
        PRINT(model);
        ospSetParam(renderer,"world",model);
        ospSetParam(renderer,"model",model);
        ospSetParam(renderer,"camera",camera);
        ospCommit(camera);
        ospCommit(renderer);
      
      };

      virtual void reshape(const ospray::vec2i &newSize)
      {
        setZUp(vec3f(0,0,1));

        Glut3DWidget::reshape(newSize);
        if (fb) ospFreeFrameBuffer(fb);
        fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
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
          viewPort.modified = false;
        }
      
        ospRenderFrame(fb,renderer);
      
        ucharFB = (uint32 *) ospMapFrameBuffer(fb);
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        Glut3DWidget::display();
      
        ospUnmapFrameBuffer(ucharFB,fb);
      
        char title[1000];

        if (alwaysRedraw) {
          sprintf(title,"OSPRay ShankarView (%f fps)",
                  fps.getFPS());
          setTitle(title);
          forceRedraw();
          glutPostRedisplay();
        } else {
          sprintf(title,"OSPRay Shankar Viewer");
          setTitle(title);
        }
      }
    
      OSPModel       model;
      OSPFrameBuffer fb;
      OSPRenderer    renderer;
      OSPCamera      camera;
      ospray::glut3D::FPSCounter fps;
    };
  
    void shankarViewerMain(int &ac, const char **&av)
    {
      shankar::Model *shankarModel = NULL;
      std::string rbfTreeFileName = "";

      cout << "shankarViewer: starting to process cmdline arguments" << endl;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "--renderer") {
          assert(i+1 < ac);
          rendererType = av[++i];
        } else if (arg == "--module" || arg == "--plugin") {
          assert(i+1 < ac);
          const char *moduleName = av[++i];
          cout << "loading ospray module '" << moduleName << "'" << endl;
          ospLoadModule(moduleName);
        } else if (av[i][0] == '-') {
          error("unkown commandline argument '"+arg+"'");
        } else {
          embree::FileName fn = arg;
          if (fn.ext() == "dat") {
            if (!shankarModel) shankarModel = new shankar::Model;
            Surface *surface = new Surface;
            surface->load(fn.str());
            shankarModel->surface.push_back(surface);
          } else if (fn.ext() == "tree") {
            rbfTreeFileName = fn.str();
          } else
              error("unknown file format "+fn.str());
        }
      }
      if ((!shankarModel || shankarModel->surface.empty()) 
          && (rbfTreeFileName == ""))
        error("no (valid?) input file(s) specified");
      
      ospRenderer = ospNewRenderer(rendererType.c_str());
      if (!ospRenderer)
        error("could not create ospRenderer '"+rendererType+"'");
      Assert(ospRenderer != NULL && "could not create ospRenderer");
    

      OSPModel model = ospNewModel();
      if (shankarModel) {
        for (int sID=0;sID<shankarModel->surface.size();sID++) {
          Surface *surface = shankarModel->surface[sID];
          OSPData data = ospNewData(surface->point.size()*7,OSP_FLOAT,
                                    &surface->point[0]);
          ospCommit(data);

          // add disks
          {
            OSPGeometry geom = ospNewGeometry("disks");
            PRINT(geom);
            ospSet1f(geom,"radius",radius);
            ospSet1i(geom,"bytes_per_disk",sizeof(shankar::Point));
            ospSet1i(geom,"center_offset",0);
            ospSet1i(geom,"normal_offset",3*sizeof(float));
            ospSet1i(geom,"radius_offset",6*sizeof(float));
            ospSetData(geom,"disks",data);
            ospCommit(geom);

            ospAddGeometry(model,geom);
          }

          // add spheres
          {
            OSPGeometry geom = ospNewGeometry("spheres");
            PRINT(geom);
            ospSet1f(geom,"radius",radius);
            ospSet1i(geom,"bytes_per_sphere",sizeof(shankar::Point));
            ospSet1i(geom,"center_offset",0);
            ospSetData(geom,"spheres",data);
            ospCommit(geom);

            ospAddGeometry(model,geom);
          }
        }
      }
      
      if (rbfTreeFileName != "") {

        OSPData nodeData = NULL;
        OSPData rbfData = NULL;

        RBFTree::load(rbfTreeFileName,rbfData,nodeData,rbfBounds);
        OSPGeometry geom = ospNewGeometry("rbfTree");
        PRINT(geom);

        ospSetData(geom,"nodes",nodeData);
        ospSetData(geom,"RBFs",rbfData);
        ospSet3fv(geom,"bounds.min",&rbfBounds.lower.x);
        ospSet3fv(geom,"bounds.max",&rbfBounds.upper.x);
        ospCommit(geom);
        
        ospAddGeometry(model,geom);
      }

      ospCommit(model);

      cout << "shankarView: done creating ospray model." << endl;

      // -------------------------------------------------------
      // create viewer window
      // -------------------------------------------------------
      ShankarViewer window(model,ospRenderer);
      window.create("ShankarViewer: OSPRay Mini-Scene Graph test viewer");
      printf("Shankar Viewer created. Press 'Q' to quit.\n");
      window.setWorldBounds(shankarModel
                            ?box3f(shankarModel->getBounds())
                            :rbfBounds);
      ospray::glut3D::runGLUT();
    }
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospLoadModule("shankar");
  ospray::glut3D::initGLUT(&ac,av);
  ospray::shankar::shankarViewerMain(ac,av);
}
