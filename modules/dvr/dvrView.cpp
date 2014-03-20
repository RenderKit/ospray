/*! \file ospDVR.cpp A GLUT-based viewer for Wavefront OBJ files */

// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;

  const char *renderType = "dvr_ispc";

  /*! \page volview_notes_on_volume_interface Internal Notes on Volume Interface

    Right now i'm using a trivially simple interface to ospray's
    volume code by simply passing filename and dimensions right into
    the ospray volume object, which then does its own parsing *inside*
    ospray. this is, however, not as it should eventually be - to be
    fixed! */

  void error(const std::string &vol)
  {
    cout << "ospray::ospDVR fatal error : " << vol << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./ospDVR <sizex> <sizey> <sizez> volFile.raw" << std::endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;

  //! volume viewer widget. 
  /*! \internal Note that all handling of camera is almost exactly
    similar to the code in msgView; might make sense to move that into
    a common class! */
  struct VolumeViewer : public Glut3DWidget {
    /*! construct volume from file name and dimensions \see volview_notes_on_volume_interface */
    VolumeViewer(const vec3i dims, const std::string &fileName, int resampleSize=0) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(NULL), volume(NULL), 
        dims(dims), fileName(fileName), resampleSize(resampleSize)
    {
      camera = ospNewCamera("perspective");
      Assert2(camera,"could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);
      ospCommit(camera);

      volume = ospNewVolume("naive32-float");
      Assert(volume && "null volume handle");
      ospSet3i(volume,"dimensions",dims.x,dims.y,dims.z);
      // ospSet3i(volume,"resample_dimensions",resampleSize,resampleSize,resampleSize);
      ospSetString(volume,"filename",fileName.c_str());
      ospCommit(volume);

      renderer = ospNewRenderer(renderType);
      Assert2(renderer,"could not create renderer");
      ospSetParam(renderer,"volume",volume);
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
      
       sprintf(title,"Test04: GlutWidget+ospray API rest (%f fps)",
               fps.getFPS());
       setTitle(title);
      forceRedraw();
    }

    std::string    fileName; /*! volume file name \see volview_notes_on_volume_interface */
    vec3i          dims;     /*! volume dimensions \see volview_notes_on_volume_interface */
    OSPVolume      volume;
    OSPFrameBuffer fb;
    OSPRenderer    renderer;
    OSPCamera      camera;
    int            resampleSize;
    ospray::glut3D::FPSCounter fps;
  };

  void ospDVRMain(int &ac, const char **&av)
  {
    ospLoadModule("dvr");

    if (ac < 5) 
      error("no input scene specified (or done so in wrong format)");
    
    vec3i volDims;
    std::string volFileName;
    int resample = 0;

    for (int i=1;i<ac;i++) {
      std::string arg = av[i];
      if (arg[0] != '-') {
        volDims.x = atoi(av[i+0]);
        volDims.y = atoi(av[i+1]);
        volDims.z = atoi(av[i+2]);
        volFileName = av[i+3];
        i += 3;
      } else if (arg == "--resample") {
        // resample volume to this size
        resample = atoi(av[++i]);
      } else if (arg == "--ispc") {
        // resample volume to this size
        renderType = "dvr_ispc";
      } else if (arg == "--scalar") {
        // resample volume to this size
        renderType = "dvr_scalar";
      } else 
        throw std::runtime_error("unknown parameter "+arg);
    }
    // const vec3i volDims(atoi(av[1]),
    //                     atoi(av[2]),
    //                     atoi(av[3]));
    // const std::string volFileName = av[4];
    
    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    VolumeViewer window(volDims,volFileName,resample);
    window.create("ospDVR: OSPRay miniature DVR volume viewer");
    printf("Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(box3f(vec3f(0.f),vec3f(resample?vec3i(resample):volDims)));
    ospray::glut3D::runGLUT();
  }
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::ospDVRMain(ac,av);
}
