/*! \file ospDVR.cpp A GLUT-based viewer for Wavefront OBJ files */

#undef NDEBUG
// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
// embree
#include "common/sys/filename.h"

namespace ospray {
  using std::cout;
  using std::endl;

  const char *renderType = "raycast";

  OSPModel model = NULL;

  void error(const std::string &err)
  {
    cout << "ospray::ospDVR fatal error : " << err << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./streamLineView <file1.pnt> <file2.pnt> ...." << std::endl;
    cout << "or" << endl;
    cout << "  ./streamLineView <listOfPntFiles.pntlist> ...." << std::endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;

  struct StreamLines {
    std::vector<vec3fa> vertex;
    std::vector<int>    index;
    float radius;

    StreamLines() : radius(.0004f) {};

    void parsePNT(const embree::FileName &fn)
    {
      FILE *file = fopen(fn.c_str(),"r");
      if (!file) {
        cout << "WARNING: could not open file " << fn << endl;
        return;
      }
      vec3fa pnt;
      static size_t totalSegments = 0;
      size_t segments = 0;
      cout << "parsing file " << fn << ":" << std::flush;
      int rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z);
      vertex.push_back(pnt);
      Assert(rc == 3);
      while ((rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z)) == 3) {
        index.push_back(vertex.size()-1);
        vertex.push_back(pnt);
        segments++;
      }
      totalSegments += segments;
      cout << " " << segments << " segments (" << totalSegments << " total)" << endl;
      fclose(file);
    }
    void parse(const embree::FileName &fn)
    {
      if (fn.ext() == "pnt") 
        parsePNT(fn);
      else if (fn.ext() == "pntlist") {
        FILE *file = fopen(fn.c_str(),"r");
        Assert(file);
        for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
          char *eol = strstr(line,"\n"); if (eol) *eol = 0;
          parsePNT(line);
        }
        fclose(file);
      } else 
        throw std::runtime_error("unknown input file format "+fn.str());
    }
    box3f getBounds() const 
    {
      box3f bounds;
      for (int i=0;i<vertex.size();i++)
        bounds.extend(vertex[i]);
      return bounds;
    }
  };

  struct StreamLineViewer : public Glut3DWidget {
    /*! construct volume from file name and dimensions \see volview_notes_on_volume_interface */
    StreamLineViewer(StreamLines *sl) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(NULL), 
        sl(sl)
    {
      camera = ospNewCamera("perspective");
      Assert2(camera,"could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);
      ospCommit(camera);

      renderer = ospNewRenderer(renderType);
      model = ospNewModel();
      OSPGeometry geom = ospNewGeometry("streamlines");
      Assert(geom);
      OSPData vertex = ospNewData(sl->vertex.size(),OSP_vec3fa,&sl->vertex[0]);
      OSPData index  = ospNewData(sl->index.size(),OSP_uint32,&sl->index[0]);
      ospSetParam(geom,"vertex",vertex);
      ospSetParam(geom,"index",index);
      ospSet1f(geom,"radius",sl->radius);
      ospAddGeometry(model,geom);
      ospCommit(model);

      Assert2(renderer,"could not create renderer");
      ospSetParam(renderer,"world",model);
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
      
      char title[1000];
      
      sprintf(title,"OSPRay StreamLines test viewer (%f fps)",
              fps.getFPS());
      setTitle(title);
      forceRedraw();
    }

    OSPFrameBuffer fb;
    OSPRenderer    renderer;
    OSPCamera      camera;
    int            resampleSize;
    ospray::glut3D::FPSCounter fps;
    StreamLines *sl;
  };

  void ospDVRMain(int &ac, const char **&av)
  {
    StreamLines *streamLines = new StreamLines;
    for (int i=1;i<ac;i++) {
      std::string arg = av[i];
      if (arg[0] != '-') {
        streamLines->parse(arg);
      } else 
        throw std::runtime_error("unknown parameter "+arg);
    }
    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    StreamLineViewer window(streamLines);
    window.create("ospDVR: OSPRay miniature stream line viewer");
    printf("Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(streamLines->getBounds());
    ospray::glut3D::runGLUT();
  }
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::ospDVRMain(ac,av);
}
