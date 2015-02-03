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

// viewer widget
#include "apps/common/widgets/glut3D.h"
// mini scene graph for loading the model
#include "miniSG/miniSG.h"
// ospray, for rendering
#include "ospray/ospray.h"

namespace ospray {
  using std::cout;
  using std::endl;
  bool doShadows = 1;

  float g_near_clip = 1e-6f;
  bool  g_fullScreen       = false;
  glut3D::Glut3DWidget::ViewPort g_viewPort;
  
  //! size of initial render window as specified via the command line.
  vec2i initWindowSize(1024,1024);

  int g_benchWarmup = 0, g_benchFrames = 0;
  bool g_alpha = false;
  bool g_createDefaultMaterial = true;
  int accumID = -1;
  int maxAccum = 64;
  int spp = 1; /*! number of samples per pixel */
  unsigned int maxObjectsToConsider = (uint32)-1;
  // if turned on, we'll put each triangle mesh into its own instance, no matter what
  bool forceInstancing = false;
  /*! if turned on we're showing the depth buffer rather than the (accum'ed) color buffer */
  bool showDepthBuffer = 0;
  glut3D::Glut3DWidget::FrameBufferMode g_frameBufferMode = glut3D::Glut3DWidget::FRAMEBUFFER_UCHAR;

  /*! when using the OBJ renderer, we create a automatic dirlight with this direction; use ''--sun-dir x y z' to change */
  //  vec3f defaultDirLight_direction(-.3, -1, .4);
  vec3f defaultDirLight_direction(.3, -1, -.2);

  Ref<miniSG::Model> msgModel = NULL;
  OSPModel           ospModel = NULL;
  OSPRenderer        ospRenderer = NULL;
  bool alwaysRedraw = false;

  //! the renderer we're about to use
  std::string rendererType = "obj";
  // std::string rendererType = "raycast_eyelight";

  std::vector<miniSG::Model *> msgAnimation;

  void error(const std::string &msg)
  {
    cout << "ospray::msgView fatal error : " << msg << endl;
    cout << endl;
    cout << "Proper usage: " << endl;
    cout << "  ./msgView [-bench <warmpup>x<numFrames>] [-model] <inFileName>" << endl;
    cout << endl;
    exit(1);
  }

  using ospray::glut3D::Glut3DWidget;
  
  /*! mini scene graph viewer widget. \internal Note that all handling
    of camera is almost exactly similar to the code in volView;
    might make sense to move that into a common class! */
  struct MSGViewer : public Glut3DWidget {
    MSGViewer(OSPModel model, OSPRenderer renderer)
      : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
        fb(NULL), renderer(renderer), model(model)
    {
      Assert(model && "null model handle");
      camera = ospNewCamera("perspective");
      Assert(camera != NULL && "could not create camera");
      ospSet3f(camera,"pos",-1,1,-1);
      ospSet3f(camera,"dir",+1,-1,+1);
      ospCommit(camera);

      ospSetObject(renderer,"world",model);
      ospSetObject(renderer,"model",model);
      ospSetObject(renderer,"camera",camera);
      ospSet1i(renderer,"spp",spp);
      ospCommit(camera);
      ospCommit(renderer);
      
    };

    virtual void reshape(const ospray::vec2i &newSize)
    {
      Glut3DWidget::reshape(newSize);
      if (fb) ospFreeFrameBuffer(fb);
      fb = ospNewFrameBuffer(newSize,OSP_RGBA_I8,OSP_FB_COLOR|OSP_FB_DEPTH|OSP_FB_ACCUM);
      ospFrameBufferClear(fb,OSP_FB_ACCUM);
      ospSetf(camera,"aspect",viewPort.aspect);
      ospCommit(camera);
      viewPort.modified = true;
      forceRedraw();
    }
    
    virtual void keypress(char key, const vec2f where)
    {
      switch (key) {
      case 'R':
        alwaysRedraw = !alwaysRedraw; 
        forceRedraw();
        break;
      case 'S':
        doShadows = !doShadows;
        cout << "Switching shadows " << (doShadows?"ON":"OFF") << endl;
        ospSet1i(renderer,"shadowsEnabled",doShadows);
        ospCommit(renderer);
        accumID=0;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case 'D':
        showDepthBuffer = !showDepthBuffer;
        // switch(g_frameBufferMode) {
        //   case Glut3DWidget::FRAMEBUFFER_DEPTH:
        //     g_frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        //     break;
        //   case Glut3DWidget::FRAMEBUFFER_UCHAR:
        //     g_frameBufferMode = Glut3DWidget::FRAMEBUFFER_DEPTH;
        //     break;
        // }
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case 'X':
        if (viewPort.up == vec3f(1,0,0) || viewPort.up == vec3f(-1.f,0,0))
          viewPort.up = - viewPort.up;
        else 
          viewPort.up = vec3f(1,0,0);
        viewPort.modified = true;
        accumID=0;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case 'Y':
        if (viewPort.up == vec3f(0,1,0) || viewPort.up == vec3f(0,-1.f,0))
          viewPort.up = - viewPort.up;
        else 
          viewPort.up = vec3f(0,1,0);
        viewPort.modified = true;
        accumID=0;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case 'Z':
        if (viewPort.up == vec3f(0,0,1) || viewPort.up == vec3f(0,0,-1.f))
          viewPort.up = - viewPort.up;
        else 
          viewPort.up = vec3f(0,0,1);
        viewPort.modified = true;
        accumID=0;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case 'f':
        {
          g_fullScreen = !g_fullScreen;
          if(g_fullScreen) glutFullScreen();
          else glutPositionWindow(0,10);
        }
        break;
      case 'r':
        {
          viewPort = g_viewPort;
        }
        break;
      default:
        Glut3DWidget::keypress(key,where);
      }
    }

    virtual void specialkey(int32 key, const vec2f where)
    {
      switch(key) {
      case GLUT_KEY_PAGE_UP:
        g_near_clip += 10.f * motionSpeed;
        ospSet1f(renderer, "near_clip", g_near_clip);
        ospCommit(renderer);
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      case GLUT_KEY_PAGE_DOWN:
        g_near_clip -= 10.f * motionSpeed;
        g_near_clip = std::max(g_near_clip, 1e-6f);
        ospSet1f(renderer, "near_clip", g_near_clip);
        ospCommit(renderer);
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
        forceRedraw();
        break;
      default:
        Glut3DWidget::keypress(key,where);
      }
    }

    virtual void mouseButton(int32 whichButton, bool released, const vec2i &pos)
    {
      Glut3DWidget::mouseButton(whichButton, released, pos);
      if(currButtonState ==  (1<<GLUT_LEFT_BUTTON) && (glutGetModifiers() & GLUT_ACTIVE_SHIFT) && manipulator == inspectCenterManipulator) {
        vec2f normpos = vec2f(pos.x / (float)windowSize.x, pos.y / (float)windowSize.y);
        OSPPickData data = ospUnproject(ospRenderer, normpos);
        vec3f p(data.world_x, data.world_y, data.world_z);
        if(data.hit) {
          vec3f delta = p - viewPort.at;
          vec3f right = cross(normalize(viewPort.at - viewPort.from), viewPort.up);
          vec3f offset = dot(delta, right) * right - dot(delta, viewPort.up) * viewPort.up;
          viewPort.at = p;
          viewPort.modified = true;
          computeFrame();
          accumID = 0;
          ospFrameBufferClear(fb,OSP_FB_ACCUM);
          forceRedraw();
        }
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
      static double benchStart=0;
      static double fpsSum=0;
      if (g_benchFrames > 0 && frameID == g_benchWarmup)
        {
          benchStart = ospray::getSysTime();
        }
      if (g_benchFrames > 0 && frameID >= g_benchWarmup)
        fpsSum += fps.getFPS();
      if (g_benchFrames > 0 && frameID== g_benchWarmup+g_benchFrames)
        {
          double time = ospray::getSysTime()-benchStart;
          double avgFps = fpsSum/double(frameID-g_benchWarmup);
          printf("Benchmark: time: %f avg fps: %f avg frame time: %f\n", time, avgFps, time/double(frameID-g_benchWarmup));
          exit(0);
        }
      
      ++frameID;
      
      if (viewPort.modified) {
        static bool once = true;
        if(once) {
          g_viewPort = viewPort;
          once = false;
        }
        Assert2(camera,"ospray camera is null");
        ospSetVec3f(camera,"pos",viewPort.from);
        ospSetVec3f(camera,"dir",viewPort.at-viewPort.from);
        ospSetVec3f(camera,"up",viewPort.up);
        ospSetf(camera,"aspect",viewPort.aspect);
        ospCommit(camera);
        viewPort.modified = false;
        accumID=0;
        ospFrameBufferClear(fb,OSP_FB_ACCUM);
      }
      
      
      ospRenderFrame(fb,renderer,OSP_FB_COLOR|(showDepthBuffer?OSP_FB_DEPTH:0)|OSP_FB_ACCUM);
      ++accumID;

      if (showDepthBuffer) {
        depthFB = (float *) ospMapFrameBuffer(fb, OSP_FB_DEPTH);
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_DEPTH;
        Glut3DWidget::display();
        ospUnmapFrameBuffer(depthFB,fb);
      } else {
        ucharFB = (uint32 *) ospMapFrameBuffer(fb, OSP_FB_COLOR);
        frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
        Glut3DWidget::display();
        ospUnmapFrameBuffer(ucharFB,fb);
      }
      // frameBufferMode = g_frameBufferMode;
      // switch(frameBufferMode) {
      //   case Glut3DWidget::FRAMEBUFFER_DEPTH:
      //     depthFB = (float *) ospMapFrameBuffer(fb, OSP_FB_DEPTH);
      //     Glut3DWidget::display();
      //     ospUnmapFrameBuffer(depthFB,fb);
      //     break;
      //   case Glut3DWidget::FRAMEBUFFER_UCHAR:
      //     ucharFB = (uint32 *) ospMapFrameBuffer(fb, OSP_FB_COLOR);
      //     Glut3DWidget::display();
      //     ospUnmapFrameBuffer(ucharFB,fb);
      //     break;
      // }

      char title[1000];

      if (alwaysRedraw) {
        sprintf(title,"OSPRay MSGView (%f fps)",
                fps.getFPS());
        setTitle(title);
        forceRedraw();
      } else if (accumID < maxAccum) {
        forceRedraw();
      } else {
        // sprintf(title,"OSPRay Model Viewer");
        // setTitle(title);
      }
    }
    
    OSPModel       model;
    OSPFrameBuffer fb;
    OSPRenderer    renderer;
    OSPCamera      camera;
    ospray::glut3D::FPSCounter fps;
  };

  void warnMaterial(const std::string &type)
  {
    static std::map<std::string,int> numOccurances;
    if (numOccurances[type] == 0)
      cout << "could not create material type '"<<type<<"'. Replacing with default material." << endl;
    numOccurances[type]++;
  }

  OSPMaterial createDefaultMaterial(OSPRenderer renderer)
  {
    if(!g_createDefaultMaterial) return NULL;
    static OSPMaterial ospMat = NULL;
    if (ospMat) return ospMat;
    
    ospMat = ospNewMaterial(renderer,"OBJMaterial");
    if (!ospMat)  {
      throw std::runtime_error("could not create default material 'OBJMaterial'");
      // cout << "given renderer does not know material type 'OBJMaterial'" << endl;
    }
    ospSet3f(ospMat, "Kd", .8f, 0.f, 0.f);
    ospCommit(ospMat);
    return ospMat;
  }

  OSPTexture2D createTexture2D(miniSG::Texture2D *msgTex)
  {
    if(msgTex == NULL) {
      static int numWarnings = 0;
      if (++numWarnings < 10)
        cout << "WARNING: material does not have Textures (only warning for the first 10 times)!" << endl;
      return NULL;
    }

    static std::map<miniSG::Texture2D*, OSPTexture2D> alreadyCreatedTextures;
    if (alreadyCreatedTextures.find(msgTex) != alreadyCreatedTextures.end())
      return alreadyCreatedTextures[msgTex];

    //TODO: We need to come up with a better way to handle different possible pixel layouts
    OSPDataType type = OSP_VOID_PTR;

    if (msgTex->depth == 1) {
      if( msgTex->channels == 3 ) type = OSP_UCHAR3;
      if( msgTex->channels == 4 ) type = OSP_UCHAR4;
    } else if (msgTex->depth == 4) {
      if( msgTex->channels == 3 ) type = OSP_FLOAT3;
      if( msgTex->channels == 4 ) type = OSP_FLOAT3A;
    }

    OSPTexture2D ospTex = ospNewTexture2D( msgTex->width,
                                           msgTex->height,
                                           type,
                                           msgTex->data,
                                           0);
    
    alreadyCreatedTextures[msgTex] = ospTex;

    ospCommit(ospTex);
    return ospTex;
  }

  OSPMaterial createMaterial(OSPRenderer renderer,
                             miniSG::Material *mat)
  {
    if (mat == NULL) {
      static int numWarnings = 0;
      if (++numWarnings < 10)
        cout << "WARNING: model does not have materials! (assigning default)" << endl;
      return createDefaultMaterial(renderer);
    }
    static std::map<miniSG::Material *,OSPMaterial> alreadyCreatedMaterials;
    if (alreadyCreatedMaterials.find(mat) != alreadyCreatedMaterials.end())
      return alreadyCreatedMaterials[mat];

    const char *type = mat->getParam("type","OBJMaterial");
    assert(type);
    OSPMaterial ospMat = alreadyCreatedMaterials[mat] = ospNewMaterial(renderer,type);
    if (!ospMat)  {
      warnMaterial(type);
      return createDefaultMaterial(renderer);
    }

    for (miniSG::Material::ParamMap::const_iterator it =  mat->params.begin();
         it !=  mat->params.end(); ++it) {
      const char *name = it->first.c_str();
      const miniSG::Material::Param *p = it->second.ptr;

      switch(p->type) {
      case miniSG::Material::Param::INT:
        ospSet1i(ospMat,name,p->i[0]);
        break;
      case miniSG::Material::Param::FLOAT:
        ospSet1f(ospMat,name,p->f[0]);
        break;
      case miniSG::Material::Param::FLOAT_3:
        ospSet3fv(ospMat,name,p->f);
        break;
      case miniSG::Material::Param::STRING:
        ospSetString(ospMat,name,p->s);
        break;
      case miniSG::Material::Param::TEXTURE:
        {
          miniSG::Texture2D *tex = (miniSG::Texture2D*)p->ptr;
          if (tex) {
            OSPTexture2D ospTex = createTexture2D(tex);
            assert(ospTex);
            ospCommit(ospTex);
            ospSetObject(ospMat, name, ospTex);
          }
          break;
        }
      default: 
        throw std::runtime_error("unkonwn material parameter type");
      };
    }

    ospCommit(ospMat);
    return ospMat;
  }

  void msgViewMain(int &ac, const char **&av)
  {
    msgModel = new miniSG::Model;
    
    cout << "msgView: starting to process cmdline arguments" << endl;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg == "--renderer") {
        assert(i+1 < ac);
        rendererType = av[++i];
      } else if (arg == "--always-redraw") {
        alwaysRedraw = true;
      } else if (arg == "--max-objects") {
        maxObjectsToConsider = atoi(av[++i]);
      } else if (arg == "--spp") {
        spp = atoi(av[++i]);
      } else if (arg == "--force-instancing") {
        forceInstancing = true;
      } else if (arg == "--pt") {
        // shortcut for '--renderer pathtracer'
        maxAccum = 1024;
        rendererType = "pathtracer";
      } else if (arg == "--sun-dir") {
        if (!strcmp(av[i+1],"none")) {
          defaultDirLight_direction = vec3f(0.f);
        } else {
          defaultDirLight_direction.x = atof(av[++i]);
          defaultDirLight_direction.y = atof(av[++i]);
          defaultDirLight_direction.z = atof(av[++i]);
        }
      } else if (arg == "--module" || arg == "--plugin") {
        assert(i+1 < ac);
        const char *moduleName = av[++i];
        cout << "loading ospray module '" << moduleName << "'" << endl;
        ospLoadModule(moduleName);
      } else if (arg == "--1k") {
        initWindowSize = vec2i(1024);
      } else if (arg == "--alpha") {
        g_alpha = true;
      } else if (arg == "-win") {
        if (++i < ac)
          {
            std::string arg2(av[i]);
            size_t pos = arg2.find("x");
            if (pos != std::string::npos)
              {
                arg2.replace(pos, 1, " ");
                std::stringstream ss(arg2);
                ss >> initWindowSize.x >> initWindowSize.y;
              }
          }
        else
          error("missing commandline param");
      } else if (arg == "-bench") {
        if (++i < ac)
          {
            std::string arg2(av[i]);
            size_t pos = arg2.find("x");
            if (pos != std::string::npos)
              {
                arg2.replace(pos, 1, " ");
                std::stringstream ss(arg2);
                ss >> g_benchWarmup >> g_benchFrames;
              }
          }
      } else if (arg == "--no-default-material") {
        g_createDefaultMaterial = false;
      } else if (av[i][0] == '-') {
        error("unkown commandline argument '"+arg+"'");
      } else {
        embree::FileName fn = arg;
        if (fn.ext() == "stl") {
          miniSG::importSTL(*msgModel,fn);
        } else if (fn.ext() == "msg") {
          miniSG::importMSG(*msgModel,fn);
        } else if (fn.ext() == "tri") {
          miniSG::importTRI(*msgModel,fn);
        } else if (fn.ext() == "xml") {
          miniSG::importRIVL(*msgModel,fn);
        } else if (fn.ext() == "obj") {
          miniSG::importOBJ(*msgModel,fn);
        } else if (fn.ext() == "x3d") {
          miniSG::importX3D(*msgModel,fn); 
        } else if (fn.ext() == "astl") {
          miniSG::importSTL(msgAnimation,fn);
        }
      }
    }

    // -------------------------------------------------------
    // done parsing
    // -------------------------------------------------------]
    cout << "msgView: done parsing. found model with" << endl;
    // cout << "  - num materials: " << msgModel->material.size() << endl;
    cout << "  - num meshes   : " << msgModel->mesh.size() << " ";
    int numUniqueTris = 0;
    int numInstancedTris = 0;
    for (int i=0;i<msgModel->mesh.size();i++) {
      if (i < 10)
        cout << "[" << msgModel->mesh[i]->size() << "]";
      else
        if (i == 10) cout << "...";
      numUniqueTris += msgModel->mesh[i]->size();
    }
    cout << endl;
    cout << "  - num instances: " << msgModel->instance.size() << " ";
    for (int i=0;i<msgModel->instance.size();i++) {
      if (i < 10)
        cout << "[" << msgModel->mesh[msgModel->instance[i].meshID]->size() << "]";
      else
        if (i == 10) cout << "...";
      numInstancedTris += msgModel->mesh[msgModel->instance[i].meshID]->size();
    }
    cout << endl;
    cout << "  - num unique triangles   : " << numUniqueTris << endl;
    cout << "  - num instanced triangles: " << numInstancedTris << endl;

    if (numInstancedTris == 0 && msgAnimation.empty())
      error("no (valid) input files specified - model contains no triangles");

    // -------------------------------------------------------
    // create ospray model
    // -------------------------------------------------------
    ospModel = ospNewModel();


    ospRenderer = ospNewRenderer(rendererType.c_str());
    if (!ospRenderer)
      throw std::runtime_error("could not create ospRenderer '"+rendererType+"'");
    Assert(ospRenderer != NULL && "could not create ospRenderer");
    ospCommit(ospRenderer);
    
    // code does not yet do instancing ... check that the model doesn't contain instances
    bool doesInstancing = 0;

    if (forceInstancing) {
      std::cout << "msgView: forced instancing - instances on." << std::endl;
      doesInstancing = true;
    } else if (msgModel->instance.size() > msgModel->mesh.size()) {
      std::cout << "msgView: found more object instances than meshes - turning on instancing" << std::endl;
      doesInstancing = true;
    } else {
      std::cout << "msgView: number of instances matches number of meshes, creating single model that contains all meshes" << std::endl;
      doesInstancing = false;
    }
    if (doesInstancing) {
      if (msgModel->instance.size() > maxObjectsToConsider) {
        cout << "cutting down on the number of meshes as requested on cmdline..." << endl;
        msgModel->instance.resize(maxObjectsToConsider);
      }
    } else {
      if (msgModel->instance.size() > maxObjectsToConsider) {
        cout << "cutting down on the number of meshes as requested on cmdline..." << endl;
        msgModel->instance.resize(maxObjectsToConsider);
        msgModel->mesh.resize(maxObjectsToConsider);
      }
    }


    cout << "msgView: adding parsed geometries to ospray model" << endl;
    std::vector<OSPModel> instanceModels;

    for (int i=0;i<msgModel->mesh.size();i++) {
      //      printf("Mesh %i/%li\n",i,msgModel->mesh.size());
      Ref<miniSG::Mesh> msgMesh = msgModel->mesh[i];

      // create ospray mesh
      OSPGeometry ospMesh = g_alpha ? ospNewGeometry("alpha_aware_triangle_mesh") : ospNewTriangleMesh();

      // check if we have to transform the vertices:
      if (doesInstancing == false && msgModel->instance[i] != miniSG::Instance(i)) {
        // cout << "Transforming vertex array ..." << endl;
        for (int vID=0;vID<msgMesh->position.size();vID++) {
          msgMesh->position[vID] = xfmPoint(msgModel->instance[i].xfm,
                                            msgMesh->position[vID]);
        }
      }
      // add position array to mesh
      OSPData position = ospNewData(msgMesh->position.size(),OSP_FLOAT3A,
                                    &msgMesh->position[0],OSP_DATA_SHARED_BUFFER);
      ospSetData(ospMesh,"position",position);
      
      // add triangle index array to mesh
      if (!msgMesh->triangleMaterialId.empty()) {
        OSPData primMatID = ospNewData(msgMesh->triangleMaterialId.size(),OSP_INT,
                                       &msgMesh->triangleMaterialId[0],OSP_DATA_SHARED_BUFFER);
        ospSetData(ospMesh,"prim.materialID",primMatID);
      }

      // cout << "INDEX" << endl;
      // add triangle index array to mesh
      OSPData index = ospNewData(msgMesh->triangle.size(),OSP_INT3,
                                 &msgMesh->triangle[0],OSP_DATA_SHARED_BUFFER);
      assert(msgMesh->triangle.size() > 0);
      ospSetData(ospMesh,"index",index);

      // add normal array to mesh
      if (!msgMesh->normal.empty()) {
        OSPData normal = ospNewData(msgMesh->normal.size(),OSP_FLOAT3A,
                                    &msgMesh->normal[0],OSP_DATA_SHARED_BUFFER);
        assert(msgMesh->normal.size() > 0);
        ospSetData(ospMesh,"vertex.normal",normal);
      } else {
        // cout << "no vertex normals!" << endl;
      }

      // add color array to mesh
      if (!msgMesh->color.empty()) {
        OSPData color = ospNewData(msgMesh->color.size(),OSP_FLOAT3A,
                                   &msgMesh->color[0],OSP_DATA_SHARED_BUFFER);
        assert(msgMesh->color.size() > 0);
        ospSetData(ospMesh,"vertex.color",color);
      } else {
        // cout << "no vertex colors!" << endl;
      }

      // add texcoord array to mesh
      if (!msgMesh->texcoord.empty()) {
        OSPData texcoord = ospNewData(msgMesh->texcoord.size(), OSP_FLOAT2,
                                      &msgMesh->texcoord[0], OSP_DATA_SHARED_BUFFER);
        assert(msgMesh->texcoord.size() > 0);
        ospSetData(ospMesh,"vertex.texcoord",texcoord);
      }

      ospSet1i(ospMesh, "alpha_type", 0);
      ospSet1i(ospMesh, "alpha_component", 4);

      // add triangle material id array to mesh
      if (msgMesh->materialList.empty()) {
        // we have a single material for this mesh...
        OSPMaterial singleMaterial = createMaterial(ospRenderer, msgMesh->material.ptr);
        ospSetMaterial(ospMesh,singleMaterial);
      } else {
        // we have an entire material list, assign that list
        std::vector<OSPMaterial > materialList;
        std::vector<OSPTexture2D > alphaMaps;
        std::vector<float> alphas;
        for (int i=0;i<msgMesh->materialList.size();i++) {
          materialList.push_back(createMaterial(ospRenderer, msgMesh->materialList[i].ptr));

          for (miniSG::Material::ParamMap::const_iterator it =  msgMesh->materialList[i]->params.begin();
               it != msgMesh->materialList[i]->params.end(); it++) {
            const char *name = it->first.c_str();
            const miniSG::Material::Param *p = it->second.ptr;
            if(p->type == miniSG::Material::Param::TEXTURE) {
              if(!strcmp(name, "map_kd") || !strcmp(name, "map_Kd")) {
                miniSG::Texture2D *tex = (miniSG::Texture2D*)p->ptr;
                OSPTexture2D ospTex = createTexture2D(tex);
                ospCommit(ospTex);
                alphaMaps.push_back(ospTex);
              }
            } else if(p->type == miniSG::Material::Param::FLOAT) {
              if(!strcmp(name, "d")) alphas.push_back(p->f[0]);
            }
          }

          while(materialList.size() > alphaMaps.size()) {
            alphaMaps.push_back(NULL);
          }
          while(materialList.size() > alphas.size()) {
            alphas.push_back(0.f);
          }
        }
        OSPData ospMaterialList = ospNewData(materialList.size(), OSP_OBJECT, &materialList[0], 0);
        ospSetData(ospMesh,"materialList",ospMaterialList);

        // only set these if alpha aware mode enabled
        // this currently doesn't work on the MICs!
        if(g_alpha) {
          OSPData ospAlphaMapList = ospNewData(alphaMaps.size(), OSP_OBJECT, &alphaMaps[0], 0);
          ospSetData(ospMesh, "alpha_maps", ospAlphaMapList);

          OSPData ospAlphaList = ospNewData(alphas.size(), OSP_OBJECT, &alphas[0], 0);
          ospSetData(ospMesh, "alphas", ospAlphaList);
        }
      }

      ospCommit(ospMesh);

      if (doesInstancing) {
        OSPModel model_i = ospNewModel();
        ospAddGeometry(model_i,ospMesh);
        ospCommit(model_i);
        instanceModels.push_back(model_i);
      } else
        ospAddGeometry(ospModel,ospMesh);

    }

    if (doesInstancing) {
      for (int i=0;i<msgModel->instance.size();i++) {
        OSPGeometry inst = ospNewInstance(instanceModels[msgModel->instance[i].meshID],
                                          msgModel->instance[i].xfm);
        ospAddGeometry(ospModel,inst);
      }
    }
    cout << "msgView: committing model" << endl;
    ospCommit(ospModel);
    cout << "msgView: done creating ospray model." << endl;

    //TODO: Need to figure out where we're going to read lighting data from
    //begin light test
    std::vector<OSPLight> dirLights;
    cout << "msgView: Adding a hard coded directional light as the sun." << endl;
    if (defaultDirLight_direction != vec3f(0.f)) {
      OSPLight ospLight = ospNewLight(ospRenderer, "DirectionalLight");
      ospSetString(ospLight, "name", "sun" );
      ospSet3f(ospLight, "color", 1, 1, 1);
      ospSet3fv(ospLight, "direction", &defaultDirLight_direction.x);
      ospCommit(ospLight);
      dirLights.push_back(ospLight);
      OSPData dirLightArray = ospNewData(dirLights.size(), OSP_OBJECT, &dirLights[0], 0);
      ospSetData(ospRenderer, "directionalLights", dirLightArray);
    }
#if 0
    //spot light
    std::vector<OSPLight> spotLights;
    cout << "msgView: Adding a hard coded spotlight for test." << endl;
    OSPLight ospSpot = ospNewLight(ospRenderer, "OBJ_SpotLight");
    ospSetString(ospSpot, "name", "spot_test");
    ospSet3f(ospSpot, "position", 0.f, 2.f, 0.f);
    ospSet3f(ospSpot, "direction", 0.f, -1.f, 0.7f);
    ospSet3f(ospSpot, "color", 1.f, 1.f, 1.f);
    ospSet1f(ospSpot, "range", 1000.f);
    ospSet1f(ospSpot, "halfAngle", 15.f);
    ospSet1f(ospSpot, "angularDropOff", 6.f);//Unused, delete?
    ospSet1f(ospSpot, "attenuation.constant", .15f);
    ospSet1f(ospSpot, "attenuation.linear", .15f);
    ospSet1f(ospSpot, "attenuation.quadratic", .15f);
    ospCommit(ospSpot);
    spotLights.push_back(ospSpot);
    OSPData spotLightArray = ospNewData(spotLights.size(), OSP_OBJECT, &spotLights[0], 0);
    ospSetData(ospRenderer, "spotLights", spotLightArray);
#endif
    //end light test
    ospCommit(ospRenderer);

    // -------------------------------------------------------
    // create viewer window
    // -------------------------------------------------------
    MSGViewer window(ospModel,ospRenderer);
    window.create("MSGViewer: OSPRay Mini-Scene Graph test viewer");
    printf("MSG Viewer created. Press 'Q' to quit.\n");
    window.setWorldBounds(box3f(msgModel->getBBox()));
    if (msgModel->camera.size() > 0) {
      window.setViewPort(msgModel->camera[0]->from,
                         msgModel->camera[0]->at,
                         msgModel->camera[0]->up);
    }
    ospray::glut3D::runGLUT();
  }
}


int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);
  ospray::msgViewMain(ac,av);
}
