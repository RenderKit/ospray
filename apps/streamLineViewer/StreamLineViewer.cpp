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

#undef NDEBUG
// viewer widget
#include "apps/common/widgets/glut3D.h"
// ospray, for rendering
#include "ospray/ospray.h"
// embree
#include "common/sys/filename.h"
// xml
#include "apps/common/xml/XML.h"

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

  void osxParseInts(std::vector<int> &vec, const std::string &content)
  {
    char *s = strdup(content.c_str());
    const char *delim = "\n\t\r ";
    char *tok = strtok(s,delim);
    while (tok) {
      vec.push_back(atol(tok));
      tok = strtok(NULL,delim);
    }
    free(s);
  }
  
  void osxParseVec3is(std::vector<vec3i> &vec, const std::string &content)
  {
    char *s = strdup(content.c_str());
    const char *delim = "\n\t\r ";
    char *tok = strtok(s,delim);
    while (tok) {
      vec3i v;
      assert(tok);
      v.x = atol(tok);
      tok = strtok(NULL,delim);

      assert(tok);
      v.y = atol(tok);
      tok = strtok(NULL,delim);

      assert(tok);
      v.z = atol(tok);
      tok = strtok(NULL,delim);

      vec.push_back(v);
    }
    free(s);
  }
  
  void osxParseVec3fas(std::vector<vec3fa> &vec, const std::string &content)
  {
    char *s = strdup(content.c_str());
    const char *delim = "\n\t\r ";
    char *tok = strtok(s,delim);
    while (tok) {
      vec3fa v;
      assert(tok);
      v.x = atof(tok);
      tok = strtok(NULL,delim);

      assert(tok);
      v.y = atof(tok);
      tok = strtok(NULL,delim);

      assert(tok);
      v.z = atof(tok);
      tok = strtok(NULL,delim);

      vec.push_back(v);
    }
    free(s);
  }
  
  /*! parse ospray xml file */
  void parseOSX(StreamLines *streamLines,
                Triangles *triangles,
                const std::string &fn)
  {
    xml::XMLDoc *doc = xml::readXML(fn);
    assert(doc);
    if (doc->child.size() != 1 || doc->child[0]->name != "OSPRay") 
      throw std::runtime_error("could not parse osx file: Not in OSPRay format!?");
    xml::Node *root_element = doc->child[0];
    for (int childID=0;childID<root_element->child.size();childID++) {
      xml::Node *node = root_element->child[childID];
      if (node->name == "Info") {
        // ignore
        continue;
      }

      if (node->name == "Model") {
        xml::Node *model_node = node;
        for (int childID=0;childID<model_node->child.size();childID++) {
          xml::Node *node = model_node->child[childID];

          if (node->name == "StreamLines") {
            
            xml::Node *sl_node = node;
            for (int childID=0;childID<sl_node->child.size();childID++) {
              xml::Node *node = sl_node->child[childID];
              if (node->name == "vertex") {
                osxParseVec3fas(streamLines->vertex,node->content);
                continue;
              };
              if (node->name == "index") {
                osxParseInts(streamLines->index,node->content);
                continue;
              };
            }
            continue;
          }
          
          if (node->name == "TriangleMesh") {
            xml::Node *tris_node = node;
            for (int childID=0;childID<tris_node->child.size();childID++) {
              xml::Node *node = tris_node->child[childID];
              if (node->name == "vertex") {
                osxParseVec3fas(triangles->vertex,node->content);
                continue;
              };
              if (node->name == "color") {
                osxParseVec3fas(triangles->color,node->content);
                continue;
              };
              if (node->name == "index") {
                osxParseVec3is(triangles->index,node->content);
                continue;
              };
            }
            continue;
          }
        }
      }
    }
  }

    void exportOSX(const char *fn,StreamLines *streamLines, Triangles *triangles)
    {
      FILE *file = fopen(fn,"w");
      fprintf(file,"<?xml version=\"1.0\"?>\n\n");
      fprintf(file,"<OSPRay>\n");
      {
        fprintf(file,"<Model>\n");
        {
          fprintf(file,"<StreamLines>\n");
          {
            fprintf(file,"<vertex>\n");
            for (int i=0;i<streamLines->vertex.size();i++)
              fprintf(file,"%f %f %f\n",
                      streamLines->vertex[i].x,
                      streamLines->vertex[i].y,
                      streamLines->vertex[i].z);
            fprintf(file,"</vertex>\n");

            fprintf(file,"<index>\n");
            for (int i=0;i<streamLines->index.size();i++)
              fprintf(file,"%i ",streamLines->index[i]);
            fprintf(file,"\n</index>\n");
          }
          fprintf(file,"</StreamLines>\n");


          fprintf(file,"<TriangleMesh>\n");
          {
            fprintf(file,"<vertex>\n");
            for (int i=0;i<triangles->vertex.size();i++)
              fprintf(file,"%f %f %f\n",
                      triangles->vertex[i].x,
                      triangles->vertex[i].y,
                      triangles->vertex[i].z);
            fprintf(file,"</vertex>\n");

            fprintf(file,"<color>\n");
            for (int i=0;i<triangles->color.size();i++)
              fprintf(file,"%f %f %f\n",
                      triangles->color[i].x,
                      triangles->color[i].y,
                      triangles->color[i].z);
            fprintf(file,"</color>\n");

            fprintf(file,"<index>\n");
            for (int i=0;i<triangles->index.size();i++)
              fprintf(file,"%i %i %i\n",
                      triangles->index[i].x,
                      triangles->index[i].y,
                      triangles->index[i].z);
            fprintf(file,"</index>\n");

          }
          fprintf(file,"</TriangleMesh>\n");
        }
        fprintf(file,"</Model>\n");
      }
      fprintf(file,"</OSPRay>\n");
      fclose(file);
    }

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
        OSPData vertex = ospNewData(sl->vertex.size(),OSP_FLOAT3A,&sl->vertex[0]);
        OSPData index  = ospNewData(sl->index.size(),OSP_UINT,&sl->index[0]);
        ospSetObject(geom,"vertex",vertex);
        ospSetObject(geom,"index",index);
        ospSet1f(geom,"radius",sl->radius);
        if (mat)
          ospSetMaterial(geom,mat);
        ospCommit(geom);
        ospAddGeometry(model,geom);
      }

      if (tris && !tris->index.empty()) {
        OSPGeometry geom = ospNewTriangleMesh();
        Assert(geom);
        OSPData vertex = ospNewData(tris->vertex.size(),OSP_FLOAT3A,&tris->vertex[0]);
        OSPData index  = ospNewData(tris->index.size(),OSP_INT3,&tris->index[0]);
        OSPData color  = ospNewData(tris->color.size(),OSP_FLOAT3A,&tris->color[0]);
        ospSetObject(geom,"vertex",vertex);
        ospSetObject(geom,"index",index);
        ospSetObject(geom,"vertex.color",color);
        ospSetMaterial(geom,mat);
        ospCommit(geom);
        ospAddGeometry(model,geom);
      }

      ospCommit(model);

      Assert2(renderer,"could not create renderer");
      ospSetObject(renderer,"world",model);
      ospSetObject(renderer,"camera",camera);
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
        if (fn.ext() == "osx")
          parseOSX(streamLines,triangles,fn);
        else if (fn.ext() == "pnt")
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
      } else if (arg == "--export") {
        exportOSX(av[++i],streamLines,triangles);
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

} // ::ospray

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::ospDVRMain(ac,av);
}
