/*! \file ongView.cpp A GLUT-based viewer for simple "transparent
    surfaces plus volume ray casting".  */

// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// mini scene graph for loading the model
#include "../../apps/util/miniSG/miniSG.h"
// ospray, for rendering
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;

  //! the renderer we're about to use
  std::string rendererType = "raycast_eyelight";
  OSPRenderer        ospRenderer = NULL;
  bool alwaysRedraw = true;

  std::string nice(size_t num)
  {
    std::stringstream ss;
    ss.setf(std::ios_base::fixed, std::ios_base::floatfield);
    ss.precision(1);
    if (num >= 1e9f)
      ss << float(num/1e9f) << "G";
    else if (num >= 1e6f)
      ss << float(num/1e6f) << "M";
    else if (num >= 1e3f)
      ss << float(num/1e3f) << "K";
    else 
      ss << num;
    return ss.str();
  }

  /*! a world of ong data, consisting of N possibly-transparent surfaces, plus a volume */
  struct OnGWorld {
    struct Surface {
      Ref<miniSG::Mesh> mesh;
      OSPGeometry       ospGeom;

      // material info
      vec3f color;
      float alpha;

      Surface() : ospGeom(NULL), color(.8f), alpha(.5f)  {};
    };

    std::vector<Surface*> surface;

    // to hold the volume data
    OSPVolume volume;
    // for the whole model: volume, plus one trianglmesh per surface
    OSPModel  model;

    OnGWorld() : volume(NULL), model(NULL) {}
    
    void load(const std::string &fn);
    box3f getBBox() {
      box3f bbox = embree::empty;
      for (int i=0;i<surface.size();i++)
        bbox.extend(surface[i]->mesh->getBBox());
      if (bbox.empty())
        throw std::runtime_error("empty bbox - did you load any geometry!?");
      return bbox;
    }
  };

  OnGWorld input;


  void error(const std::string &msg)
  {
    cout << "ospray::ongView fatal error : " << msg << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./ongView [-bench <warmpup>x<numFrames>] [-model] <inFileName>" << endl;
    cout << endl;
    exit(1);
  }

  void OnGWorld::load(const std::string &fn)
  {
    embree::FileName inputFileName(fn);

    if (!surface.empty())
      throw std::runtime_error("did you try to load more than one ong file!?");

    FILE *file = fopen(fn.c_str(),"r");
    if (!file)
      throw std::runtime_error("could not open input file '"+fn+"'");
      
    char line[10000];
    long totalNumTriangles = 0;
    while (fgets(line,10000,file) && !feof(file)) {
      char token[1000];
      strcpy(token,"");
      sscanf(line,"%s",token);

      if (std::string(token) == "surface") {
        Surface *surf = new Surface;
        char surfFileName[10000];
        int nRead = sscanf(line,"surface %s color %f %f %f alpha %f",
                           surfFileName,
                           &surf->color.x,
                           &surf->color.y,
                           &surf->color.z,
                           &surf->alpha);
        if (nRead != 5) throw std::runtime_error("could not parse line: "+std::string(line));

        cout << "#ongView:loading surface " << surfFileName << "..." << std::flush;
        embree::FileName fn = inputFileName.path().str()+"/"+surfFileName;
        miniSG::Model msgModel;
        if (fn.ext() == "stl")
          miniSG::importSTL(msgModel,fn);
        else if (fn.ext() == "msg")
          miniSG::importMSG(msgModel,fn);
        else if (fn.ext() == "tri")
          miniSG::importTRI(msgModel,fn);
        else if (fn.ext() == "xml")
          miniSG::importRIVL(msgModel,fn);
        else if (fn.ext() == "obj")
          miniSG::importOBJ(msgModel,fn);
        if (msgModel.mesh.size() != 1) {
          throw std::runtime_error("invalid surface model "+fn.str()+
                                   " (must contain exactly one mesh!)");
        }
        surf->mesh = msgModel.mesh[0];
        cout << nice(surf->mesh->triangle.size()) << " triangles." << endl;
        totalNumTriangles += surf->mesh->triangle.size();
        surface.push_back(surf);
        continue;
      }
    }
    cout << "#ongView:total num triangles read: " << nice(totalNumTriangles) << endl;
  }


  using ospray::glut3D::Glut3DWidget;
  
  /*! mini scene graph viewer widget. \internal Note that all handling
    of camera is almost exactly similar to the code in volView;
    might make sense to move that into a common class! */
  struct ONGViewer : public Glut3DWidget {
    ONGViewer(OSPModel model, OSPRenderer renderer)
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(renderer), model(model)
    {
      Assert(model && "null model handle");
      camera = ospNewCamera("perspective");
      Assert(camera != NULL && "could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);

      ospSetParam(renderer,"world",model);
      ospSetParam(renderer,"model",model);
      ospSetParam(renderer,"camera",camera);
      ospCommit(camera);
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
        sprintf(title,"OSPRay ONGView (%f fps)",
                fps.getFPS());
        setTitle(title);
        forceRedraw();
      } else {
        sprintf(title,"OSPRay ONGView");
        setTitle(title);
      }
    }
    
    OSPModel       model;
    OSPFrameBuffer fb;
    OSPRenderer    renderer;
    OSPCamera      camera;
    ospray::glut3D::FPSCounter fps;
  };
  
  void ongViewMain(int &ac, const char **&av)
  {
    cout << "#ongView: starting to process cmdline arguments" << endl;
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
        input.load(arg);
      }
    }

    if (input.surface.empty())
      throw std::runtime_error("no input specified (or empty input model)");

    // -------------------------------------------------------
    // create ospray model
    // -------------------------------------------------------
    OSPModel ospModel = input.model = ospNewModel();


    ospRenderer = ospNewRenderer(rendererType.c_str());
    if (!ospRenderer)
      throw std::runtime_error("could not create ospRenderer '"+rendererType+"'");
    Assert(ospRenderer != NULL && "could not create ospRenderer");
    

    cout << "ongView: defining " << input.surface.size() << " surfaces" << endl;
    for (int i=0;i<input.surface.size();i++) {
      OnGWorld::Surface *surface = input.surface[i];
      Ref<miniSG::Mesh> ongMesh = surface->mesh;

      // create ospray mesh
      OSPGeometry ospMesh = ospNewTriangleMesh();

      // add position array to mesh
      OSPData position = ospNewData(ongMesh->position.size(),OSP_vec3fa,
                                    &ongMesh->position[0],OSP_DATA_SHARED_BUFFER);
      ospSetData(ospMesh,"position",position);
      
      // add triangle index array to mesh
      OSPData index = ospNewData(ongMesh->triangle.size(),OSP_vec3i,
                                 &ongMesh->triangle[0],OSP_DATA_SHARED_BUFFER);
      assert(ongMesh->triangle.size() > 0);
      ospSetData(ospMesh,"index",index);

      // add normal array to mesh
      if (!ongMesh->normal.empty()) {
        OSPData normal = ospNewData(ongMesh->normal.size(),OSP_vec3fa,
                                    &ongMesh->normal[0],OSP_DATA_SHARED_BUFFER);
        assert(ongMesh->normal.size() > 0);
        ospSetData(ospMesh,"vertex.normal",normal);
      } else {
        cout << "no vertex normals!" << endl;
      }
      
      // -------------------------------------------------------
      // set material
      // -------------------------------------------------------
      OSPMaterial ospMaterial = ospNewMaterial(ospRenderer,"ongsandbox");
      if (ospMaterial) {
        ospSet3fv(ospMaterial,"color",&surface->color.x);
        ospSet1f(ospMaterial,"alpha",surface->alpha);
        ospCommit(ospMaterial);
        ospSetMaterial(ospMesh,ospMaterial);
      }

      ospCommit(ospMesh);
      ospAddGeometry(ospModel,ospMesh);
    }

    // TODO: load the volume ...

    ospCommit(ospModel);
    cout << "#ongView: done creating ospray model." << endl;

    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    ONGViewer window(ospModel,ospRenderer);
    window.create("ONGViewer: OSPRay Mini-Scene Graph test viewer");
    printf("ONG Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(box3f(input.getBBox()));
    PING;
    ospray::glut3D::runGLUT();
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospLoadModule("ongsandbox");
  ospray::glut3D::initGLUT(&ac,av);
  ospray::ongViewMain(ac,av);
}
