/*! \file hairViewer.cpp A GLUT-based viewer for simple geometry
    (supports STL and Wavefront OBJ files) */

#undef NDEBUG

// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// mini scene graph for loading the model
#include "hairgeom.h"
// ospray, for rendering
#include "ospray/ospray.h"
// emnbree
#include "tutorial/tutorial.h"

namespace ospray {
  using std::cout;
  using std::endl;

  OSPModel           ospModel = NULL;

  namespace hair {
    extern long numIsecs;
    extern long numIsecRecs;
    extern long numHairletTravs;
  }
  
  void error(const std::string &msg)
  {
    cout << "ospray::msgView fatal error : " << msg << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./msgView <inFileName>" << endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;

  /*! mini scene graph viewer widget. \internal Note that all handling
      of camera is almost exactly similar to the code in volView;
      might make sense to move that into a common class! */
  struct MSGViewer : public Glut3DWidget {
    MSGViewer(OSPModel model) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(NULL), model(model)
    {
      Assert(model && "null model handle");
      camera = ospNewCamera("perspective");
      Assert(camera != NULL && "could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);
      
      // renderer = ospNewRenderer("ray_cast");
      renderer = ospNewRenderer("raycast_geomID");
      Assert(renderer != NULL && "could not create renderer");

      ospSetParam(renderer,"world",model);
      ospSetParam(renderer,"camera",camera);
      ospCommit(camera);
      ospCommit(renderer);

    };
    virtual void reshape(const ospray::vec2i &newSize) 
    {
      Glut3DWidget::reshape(newSize);
      if (fb) ospFreeFrameBuffer(fb);
      fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8);
      ospSet1f(camera,"aspect",viewPort.aspect);
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
        // PRINT(viewPort);
        ospSetVec3f(camera,"pos",viewPort.from);
        ospSetVec3f(camera,"dir",viewPort.at-viewPort.from);
        ospSetVec3f(camera,"up",viewPort.up);
        ospSet1f(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
        viewPort.modified = false;
      }

      ospRenderFrame(fb,renderer);
    
      ucharFB = (uint32 *) ospMapFrameBuffer(fb);
      frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
      Glut3DWidget::display();
    
      ospUnmapFrameBuffer(ucharFB,fb);

      char title[1000];
      
      sprintf(title,"Test04: GlutWidget+ospray API rest (%f fps)",
              fps.getFPS());
      setTitle(title);

      if (hair::numIsecs) {
        PRINT(hair::numIsecs);
        PRINT(hair::numIsecRecs);
        PRINT(hair::numHairletTravs);
      }
      hair::numIsecs = 0;
      hair::numIsecRecs = 0;
      hair::numHairletTravs = 0;

      forceRedraw();
    }

    OSPModel       model;
    OSPFrameBuffer fb;
    OSPRenderer    renderer;
    OSPCamera      camera;
    ospray::glut3D::FPSCounter fps;
  };

  void hairViewMain(int &ac, const char **&av)
  {
    // -------------------------------------------------------
    // parse cmdline
    // -------------------------------------------------------
    cout << "hairViewer: starting to process cmdline arguments" << endl;
    // for (int i=1;i<ac;i++) {
    //   const std::string arg = av[i];
    //   if (av[i][0] == '-') {

    //     error("unkown commandline argument '"+arg+"'");
    //   } else {
    //     embree::FileName fn = arg;
    //     if (fn.ext() == "stl")
    //       miniSG::importSTL(*msgModel,fn);
    //     else if (fn.ext() == "msg")
    //       miniSG::importMSG(*msgModel,fn);
    //     else if (fn.ext() == "obj")
    //       miniSG::importOBJ(*msgModel,fn);
    //     else if (fn.ext() == "astl")
    //       miniSG::importSTL(msgAnimation,fn);
    //     else
    //       error("unrecognized file format in filename '"+arg+"'");
    //   }
    // }
    if (ac != 2)
      error("hairViewer file.hbvh");
    const char *hbvhFileName = av[1];
    embree::FileName hgFileName = embree::FileName(hbvhFileName).setExt(".hg");//av[1];
    PRINT(hbvhFileName);
    PRINT(hgFileName);
    // -------------------------------------------------------
    // done parsing
    // -------------------------------------------------------
    
    cout << "done parsing cmdline args, loading hg file from " << hgFileName << endl;
    hair::HairGeom *hg = new hair::HairGeom;
    hg->load(hgFileName);
    cout << "Transforming hair geometry to origin ..." << endl;
    // box4f one(vec4f(0.f),vec4f(1.f));
    box4f orgSpace = hg->bounds;
    PRINT(orgSpace);
    hg->transformToOrigin();
    PRINT(hg->bounds);
    cout << "hg file loaded, (translated) bounds is " << hg->bounds << endl;

    // -------------------------------------------------------
    // create ospray model
    // -------------------------------------------------------
    ospModel = ospNewModel();
    ospLoadModule("hair");
    OSPGeometry ospHair = ospNewGeometry("hair_bvh");
    Assert(ospHair);
    cout << "setting void ptr 'hg' to " << hg << endl;
    ospSetVoidPtr(ospHair,"hg",hg);
    ospSetString(ospHair,"hbvh",hbvhFileName);
    ospSet1i(ospHair,"max_bricks",-1);
    ospAddGeometry(ospModel,ospHair);
    ospCommit(ospModel);
    cout << "hairViewer: done creating ospray model." << endl;

    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    MSGViewer window(ospModel);
    window.create("HairViewer: OSPRay Hair Module test viewer");
    printf("Hair Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(box3f((vec3f&)(hg->bounds.lower),(vec3f&)(hg->bounds.upper)));
    window.setZUp(vec3f(0,1,0));

    ospray::glut3D::runGLUT();
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::hairViewMain(ac,av);
}
