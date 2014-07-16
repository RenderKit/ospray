/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */




/*
 mm -C ~/Projects/ospray/bin/ && ~/Projects/ospray/bin/streamLineView ~/models/NASA/B-field-sun/ext/*pnt ~/models/NASA/B-field-sun/original/temp\=6400k.sv   --renderer obj --1k -vp 0.897989 0.588022 -0.800192 -vi 0.0106688 -0.212129 -0.029311 -vu 0 1 0 
*/

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

  const char *rendererType = "raycast_eyelight";
  bool doShadows = 1;

  OSPModel model = NULL;

  void error(const std::string &err)
  {
    cout << "ospray::ospDVR fatal error : " << err << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./streamLineView <file1.pnt> <file2.pnt> ....<fileN.sv>" << std::endl;
    cout << "or" << endl;
    cout << "  ./streamLineView <listOfPntFiles.pntlist> ...." << std::endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;

  struct Triangles {
    std::vector<vec3fa> vertex;
    std::vector<vec3fa> color; // vertex color, from sv's 'v' value
    std::vector<vec3i>  index;

    struct SVVertex {
      float v;
      vec3f pos; //float x,y,z;
    };

    struct SVTriangle {
      SVVertex vertex[3];
    };

    vec3f lerpf(float x, float x0,float x1,vec3f y0, vec3f y1)
    {
      float f = (x-x0)/(x1-x0);
      return f*y1+(1-f)*y0;
    }
    vec3f colorOf(const float f)
    {
      if (f < .5f)
        return vec3f(lerpf(f, 0.f,.5f,vec3f(0),vec3f(0,1,0)));
      else
        return vec3f(lerpf(f, .5f,1.f,vec3f(0,1,0),vec3f(1,0,0)));
    }
    void parseSV(const embree::FileName &fn)
    {
      FILE *file = fopen(fn.str().c_str(),"rb");
      if (!file) return;
      SVTriangle triangle;
      while (fread(&triangle,sizeof(triangle),1,file)) {
        index.push_back(vec3i(0,1,2)+vec3i(vertex.size()));
        vertex.push_back(vec3fa(triangle.vertex[0].pos));
        vertex.push_back(vec3fa(triangle.vertex[1].pos));
        vertex.push_back(vec3fa(triangle.vertex[2].pos));
        color.push_back(colorOf(triangle.vertex[0].v));
        color.push_back(colorOf(triangle.vertex[1].v));
        color.push_back(colorOf(triangle.vertex[2].v));
      }
      fclose(file);
    }
  };

  struct StreamLines {
    std::vector<vec3fa> vertex;
    std::vector<int>    index;
    float radius;

    StreamLines() : radius(0.001f) {};

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
      // cout << "parsing file " << fn << ":" << std::flush;
      int rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z);
      vertex.push_back(pnt);
      Assert(rc == 3);
      while ((rc = fscanf(file,"%f %f %f\n",&pnt.x,&pnt.y,&pnt.z)) == 3) {
        index.push_back(vertex.size()-1);
        vertex.push_back(pnt);
        segments++;
      }
      totalSegments += segments;
      // cout << " " << segments << " segments (" << totalSegments << " total)" << endl;
      fclose(file);
    }
    void parsePNTlist(const embree::FileName &fn)
    {
      FILE *file = fopen(fn.c_str(),"r");
      Assert(file);
      for (char line[10000]; fgets(line,10000,file) && !feof(file); ) {
        char *eol = strstr(line,"\n"); if (eol) *eol = 0;
        parsePNT(line);
      }
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
      box3f bounds = embree::empty;
      for (int i=0;i<vertex.size();i++)
        bounds.extend(vertex[i]);
      return bounds;
    }
  };

  struct StreamLineViewer : public Glut3DWidget {
    /*! construct volume from file name and dimensions \see volview_notes_on_volume_interface */
    StreamLineViewer(StreamLines *sl, Triangles *tris) 
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(NULL), 
        sl(sl), tris(tris)
    {
      camera = ospNewCamera("perspective");
      Assert2(camera,"could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);
      ospCommit(camera);

      renderer = ospNewRenderer(rendererType);

      OSPMaterial mat = ospNewMaterial(renderer,"default");
      if (mat) {
        ospSet3f(mat,"kd",.7,.7,.7); // OBJ renderer, default ...
        ospCommit(mat);
      }
      
      if (std::string(rendererType) == "obj") {
        OSPLight topLight = ospNewLight(renderer,"DirectionalLight");
        if (topLight) {
          ospSet3f(topLight,"direction",-1,-2,1);
          ospSet3f(topLight,"color",1,1,1);
          ospCommit(topLight);
          OSPData lights = ospNewData(1,OSP_OBJECT,&topLight);
          ospCommit(lights);
          ospSetData(renderer,"directionalLights",lights);
        }
      } 

      ospCommit(renderer);

      model = ospNewModel();
      {
        OSPGeometry geom = ospNewGeometry("streamlines");
        Assert(geom);
        OSPData vertex = ospNewData(sl->vertex.size(),OSP_vec3fa,&sl->vertex[0]);
        OSPData index  = ospNewData(sl->index.size(),OSP_uint32,&sl->index[0]);
        ospSetParam(geom,"vertex",vertex);
        ospSetParam(geom,"index",index);
        ospSet1f(geom,"radius",sl->radius);
        if (mat)
          ospSetMaterial(geom,mat);
        ospCommit(geom);
        ospAddGeometry(model,geom);
      }

      if (tris && !tris->index.empty()) {
        OSPGeometry geom = ospNewTriangleMesh();
        Assert(geom);
        OSPData vertex = ospNewData(tris->vertex.size(),OSP_vec3fa,&tris->vertex[0]);
        OSPData index  = ospNewData(tris->index.size(),OSP_vec3i,&tris->index[0]);
        OSPData color  = ospNewData(tris->color.size(),OSP_vec3fa,&tris->color[0]);
        ospSetParam(geom,"vertex",vertex);
        ospSetParam(geom,"index",index);
        ospSetParam(geom,"vertex.color",color);
        ospSetMaterial(geom,mat);
        ospCommit(geom);
        ospAddGeometry(model,geom);
      }

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
      fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8,OSP_FB_COLOR|OSP_FB_ACCUM);
      ospSetf(camera,"aspect",viewPort.aspect);
      ospCommit(camera);
    }

    virtual void keypress(char key, const vec2f where)
    {
      switch (key) {
      case 'S':
        doShadows = !doShadows;
        cout << "Switching shadows " << (doShadows?"ON":"OFF") << endl;
        ospSet1i(renderer,"shadowsEnabled",doShadows);
        ospCommit(renderer);
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        break;
      default:
        Glut3DWidget::keypress(key,where);
      }
    }
    virtual void display() 
    {
      if (!fb || !renderer) return;

      if (viewPort.modified) {
        Assert2(camera,"ospray camera is null");
        
        ospSetVec3f(camera,"pos",viewPort.from);
        ospSetVec3f(camera,"dir",viewPort.at-viewPort.from);
        ospSetVec3f(camera,"up",viewPort.up);
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
        viewPort.modified = false;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
      }

      fps.startRender();
      ospRenderFrame(fb,renderer,OSP_FB_COLOR|OSP_FB_ACCUM
                     );
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
    Triangles *tris;
  };

  void ospDVRMain(int &ac, const char **&av)
  {
    StreamLines *streamLines = new StreamLines;
    Triangles   *triangles = new Triangles;
    for (int i=1;i<ac;i++) {
      std::string arg = av[i];
      if (arg[0] != '-') {
        const embree::FileName fn = arg;
        if (fn.ext() == "pnt")
          streamLines->parsePNT(fn);
        else if (fn.ext() == "pntlist")
          streamLines->parsePNTlist(fn);
        else if (fn.ext() == "sv") 
          triangles->parseSV(fn);
        else
          throw std::runtime_error("unknown file format "+fn.str());
      } else if (arg == "--module") {
	ospLoadModule(av[++i]);
      } else if (arg == "--renderer") {
        rendererType = av[++i];
      } else if (arg == "--radius") {
        streamLines->radius = atof(av[++i]);
      } else
        throw std::runtime_error("unknown parameter "+arg);
    }
    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    StreamLineViewer window(streamLines,triangles);
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
