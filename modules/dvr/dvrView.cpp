/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



#undef NDEBUG
// viewer widget
#include "../../apps/util/glut3D/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;

  const char *renderType = "dvr_ispc";
  const char *volumeLayout = "naive32";
  std::string inputFormat = "uint8";
  const char *resampleFormat = NULL;
  int         resampleSize = -1;
  float       dt = 0.f; // dt as set from cmdline; '0.f' means auto-generate

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
    cout << "  ./ospDVR uint8|float <sizex> <sizey> <sizez> volFile.raw [options]" << std::endl;
    cout << "options:" << endl;
    cout << " --ispc                      : use ISPC renderer" << endl;
    cout << " --scalar                    : use scalar renderer" << endl;
    cout << " --naive                     : use naive z-order data layout" << endl;
    cout << " --bricked                   : use bricked data layout" << endl;
    cout << " --page-bricked              : use paged-and-bricked data layout" << endl;
    cout << " --resample uint8|float size : resample to given format and size^3" << endl;
    cout << " -dt <dt>                    : use ray cast sample step size 'dt'" << endl;
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
    VolumeViewer(const vec3i dims, const std::string &fileName) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(NULL), volume(NULL), 
        dims(dims), fileName(fileName)
    {
      camera = ospNewCamera("perspective");
      Assert2(camera,"could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);
      ospCommit(camera);

      if (resampleFormat) {
        std::string volumeType 
          = std::string("naive32")
          + "_"
          + std::string(inputFormat);
        PRINT(volumeType);
        OSPVolume tmp = ospNewVolume(volumeType.c_str());
        ospSet3i(tmp,"dimensions",dims.x,dims.y,dims.z);
        ospSetString(tmp,"filename",fileName.c_str());
        ospCommit(tmp);
        PRINT(tmp);
        volumeType 
          = std::string(volumeLayout)
          + "_"
          + std::string(resampleFormat);
        volume = ospNewVolume(volumeType.c_str());
        PRINT(resampleSize);
        ospSet3i(volume,"dimensions",resampleSize,resampleSize,resampleSize);
        ospSetParam(volume,"resample_source",tmp);
        ospCommit(volume);
      } else {
        std::string volumeType 
          = std::string(volumeLayout)
          + "_"
          + std::string(inputFormat);
        volume = ospNewVolume(volumeType.c_str());
        ospSet3i(volume,"dimensions",dims.x,dims.y,dims.z);
        ospSetString(volume,"filename",fileName.c_str());
        ospCommit(volume);
      }

      renderer = ospNewRenderer(renderType);
      if (dt != 0.f) {
        cout << "using step size " << dt << endl;
      } else {
        dt = 0.5f/256;//1f/std::max(dims.x,std::max(dims.y,dims.z)));
        cout << "using default step size " << dt << endl;
      }
      ospSet1f(renderer,"dt",dt);

      Assert2(renderer,"could not create renderer");
      ospSetParam(renderer,"volume",volume);
      ospSetParam(renderer,"camera",camera);
      ospCommit(renderer);

    };
    virtual void keypress(char key, const vec2f where)
    {
      switch (key) {
      case '>':
        dt /= 1.5f;
        cout << "#osp:dvr: new step size " << dt << endl;
        ospSet1f(renderer,"dt",dt);
        ospCommit(renderer);
        break;
      case '<':
        dt *= 1.5f;
        cout << "#osp:dvr: new step size " << dt << endl;
        ospSet1f(renderer,"dt",dt);
        ospCommit(renderer);
        break;
      default:
        Glut3DWidget::keypress(key,where);
      }
    }
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
    ospray::glut3D::FPSCounter fps;
  };

  void ospDVRMain(int &ac, const char **&av)
  {
    ospLoadModule("dvr");

    if (ac < 5) 
      error("no input scene specified (or done so in wrong format)");
    
    vec3i volDims;
    std::string volFileName;

    for (int i=1;i<ac;i++) {
      std::string arg = av[i];
      if (arg[0] != '-') {
        inputFormat = strdup(av[i+0]);
        if (inputFormat != "uint8" && inputFormat != "float")
          error("unknown raw folume file scalar type '"+std::string(inputFormat)+"'");
        volDims.x = atoi(av[i+1]);
        volDims.y = atoi(av[i+2]);
        volDims.z = atoi(av[i+3]);
        volFileName = av[i+4];
        i += 4;
      } else if (arg == "-dt") {
        dt = atof(av[i+1]);
        i += 1;
      } else if (arg == "--resample") {
        // resample volume to this size
        resampleFormat = strdup(av[++i]);
        resampleSize   = atoi(av[++i]);
      } else if (arg == "--ispc") {
        // resample volume to this size
        renderType = "dvr_ispc";
      } else if (arg == "--scalar") {
        // resample volume to this size
        throw std::runtime_error("scalar dvr renderer currently disabled; use ispc variant");
        //renderType = "dvr_scalar";
      } else if (arg == "--naive") {
        // use naive32 volume layout
        volumeLayout = "naive32";
      } else if (arg == "--bricked") {
        // use bricked32 volume layout
        volumeLayout = "bricked32";
      } else if (arg == "--page-bricked") {
        // use bricked32 volume layout
        volumeLayout = "pageBricked64";
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
    VolumeViewer window(volDims,volFileName);
    window.create("ospDVR: OSPRay miniature DVR volume viewer");
    printf("Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(box3f(vec3f(0.f),vec3f(1.f)));
    // window.setWorldBounds(box3f(vec3f(0.f),vec3f(resample?vec3i(resample):volDims)));
    ospray::glut3D::runGLUT();
  }
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::ospDVRMain(ac,av);
}
