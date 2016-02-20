// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#pragma once

// ospray PUBLIC api
#include "ospray/ospray.h"
// ospcommon
#include "common/common.h"
#include "common/RefCount.h"
#include "common/box.h"
#include "common/AffineSpace.h"
#include "common/FileName.h"

// // embree 
// #include "common/sys/filename.h"
// stl 
#include <vector>
#include <map>

namespace ospray {
  namespace miniSG {
    using namespace ospcommon;
    typedef ospcommon::AffineSpace3f affine3f;

    struct Camera : public RefCount {
      vec3f from, at, up;
    };

    struct Texture2D : public RefCount {
      Texture2D();

      int channels; //Number of color channels per pixel
      int depth;    //Bytes per color channel
      bool prefereLinear; //A linear texel format is preferred over sRGB
      int width;    //Pixels per row
      int height;   //Pixels per column
      void *data;   //Pointer to binary texture data
    };
    
    Texture2D *loadTexture(const std::string &path, const std::string &fileName, const bool prefereLinear = false);

    struct Material : public RefCount {
      struct Param : public RefCount {
        typedef enum {
          INT,
          INT_2,
          INT_3,
          INT_4,
          UINT,
          UINT_2,
          UINT_3,
          UINT_4,
          FLOAT,
          FLOAT_2,
          FLOAT_3,
          FLOAT_4,
          TEXTURE,
          STRING,
          UNKNOWN,
        } DataType;

        void clear() {
          Assert2(this != NULL, "Tried to clear a null parameter");
          switch( type ) {
            case STRING:
              if (s) free((void*)s);
              s = NULL;
              break;
            case TEXTURE:
              ptr = NULL;
              break;
            default:
              type = TEXTURE;
              ptr = NULL;
          };
        }

        void set(const char *v) { 
          clear();
          s = strdup(v);
          type = STRING;
          // char *str = (char*)malloc(strlen(v));
          // strcpy(str, v);
          // s = str;
        }

        void set(void *v, DataType t = UNKNOWN) { clear(); type = t; ptr = v; }

        void set(float v) { clear(); type = FLOAT; f[0] = v; }
        void set(vec2f v) { clear(); type = FLOAT_2; f[0] = v.x; f[1] = v.y; }
        void set(vec3f v) { clear(); type = FLOAT_3; f[0] = v.x; f[1] = v.y; f[2] = v.z; }
        void set(vec4f v) { clear(); type = FLOAT_3; f[0] = v.x; f[1] = v.y; f[2] = v.z; f[3] = v.w; }

        void set(int32_t v) { clear(); type = INT; i[0] = v; }
        void set(vec2i v) { clear(); type = INT_2; i[0] = v.x; i[1] = v.y; }
        void set(vec3i v) { clear(); type = INT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; }
        void set(vec4i v) { clear(); type = INT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; i[3] = v.w; }

        void set(uint32_t v) { clear(); type = UINT; i[0] = v; }
        void set(vec2ui v) { clear(); type = UINT_2; i[0] = v.x; i[1] = v.y; }
        void set(vec3ui v) { clear(); type = UINT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; }
        void set(vec4ui v) { clear(); type = UINT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; i[3] = v.w; }

        Param() {
          s = NULL;
          type = INT;
        }
        union {
          float      f[4];
          int32_t      i[4];
          uint32_t     ui[4];
          const char *s;
          void       *ptr;
        };

        DataType type;
      };

      typedef std::map< std::string, Ref<Param> > ParamMap;

      Material();

      float getParam(const char *name, float defaultVal);
      vec2f getParam(const char *name, vec2f defaultVal);
      vec3f getParam(const char *name, vec3f defaultVal);
      vec4f getParam(const char *name, vec4f devaultVal);

      int32_t getParam(const char *name, int32_t defaultVal);
      vec2i getParam(const char *name, vec2i defaultVal);
      vec3i getParam(const char *name, vec3i defaultVal);
      vec4i getParam(const char *name, vec4i defaultVal);

      uint32_t getParam(const char *name, uint32_t defaultVal);
      vec2ui getParam(const char *name, vec2ui defaultVal);
      vec3ui getParam(const char *name, vec3ui defaultVal);
      vec4ui getParam(const char *name, vec4ui defaultVal);

      const char *getParam(const char *name, const char *defaultVal);
      void *getParam(const char *name, void *defaultVal);

      //At least we can be lazy when setting parameters
      template< typename T >
        void setParam(const char *name, T v) {
          //Clean up old parameter
          ParamMap::iterator it = params.find(name);
          if(it != params.end() ) params.erase(it);
          Param *p = new Param;
          p->set(v);
          params[name] = p;
        }

      void setParam(const char *name, void *v, Param::DataType t = Param::TEXTURE) {
        ParamMap::iterator it = params.find(name);
        if(it != params.end() ) params.erase(it);
        Param *p = new Param;
        p->set(v, t);
        params[name] = p;
      }

      std::string toString() const { return "miniSG::Material"; }

      ParamMap params;
      std::vector<Ref<Texture2D> > textures;
      std::string name;
      std::string type;
    };

    struct Triangle {
      uint32_t v0, v1, v2;
    };

    /*! default triangle mesh layout */
    struct Mesh : public RefCount {
      std::string           name;     /*!< symbolic name of mesh, can be empty */
      std::vector<vec3fa>   position; /*!< vertex positions */
      std::vector<vec3fa>   normal;   /*!< vertex normals; empty if none present */
      std::vector<vec3fa>   color ;   /*!< vertex colors; empty if none present */
      std::vector<vec2f>    texcoord; /*!< vertex texcoords; empty if none present */
      std::vector<Triangle> triangle; /*!< triangles' vertex IDs */
      std::vector<Ref<Material> > materialList; /*!< entire list of
                                             materials, in case the
                                             mesh has per-primitive
                                             material IDs (list may be
                                             empty, in which case the
                                             'material' field
                                             applies */

      /*! per-primitive material ID, if applicable. if empty every
          triangle uses the 'material' field of the mesh; if not emtpy
          the ID in here refers as a index into the
          'materialList'. Will eventually get merged into the foruth
          component of the triangle, but right now ospray/embree do
          not yet allow this ... */
      std::vector<uint32_t> triangleMaterialId;
      
      
      box3f bounds; /*!< bounding box of all vertices */

      int size() const { return triangle.size(); }
      Ref<Material> material;
      box3f getBBox();
      Mesh() : bounds(ospcommon::empty) {};
    };

    struct Instance : public RefCount {
      Instance() : meshID(0), xfm(ospcommon::one), ospGeometry(NULL) {}
      Instance(int meshID, affine3f xfm=ospcommon::one) 
        : meshID(meshID), xfm(xfm), ospGeometry(NULL) 
      {};
      Instance(const Instance &o)
        : meshID(o.meshID), xfm(o.xfm), ospGeometry(o.ospGeometry)
      {}

      affine3f xfm;
      int meshID;
      
      OSPGeometry ospGeometry;
    };

    bool operator==(const Instance &a, const Instance &b);
    bool operator!=(const Instance &a, const Instance &b);

    struct Model : public RefCount {
      /*! list of meshes that the scene is composed of */
      std::vector<Ref<Mesh> >     mesh;
      /*! \brief list of instances (if available). */
      std::vector<Instance>       instance;
      /*! \brief list of camera defined in the model (usually empty) */
      std::vector<Ref<Camera> >   camera;

      //! return number of meshes in this model
      inline size_t numMeshes() const { return mesh.size(); }
      //! return number of unique triangles (ie, _ex_ instantiation!) in this model
      size_t numUniqueTriangles() const;
      /*! computes and returns the world-space bounding box of the entire model */
      box3f getBBox();
    };

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importOBJ(Model &model, const FileName &fileName);

    /*! import a HBP file, and add it to the specified model */
    void importHBP(Model &model, const FileName &fileName);

    /*! import a TRI file (format:vec3fa[3][numTris]), and add it to the specified model */
    void importTRI(Model &model, const FileName &fileName);

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importRIVL(Model &model, const FileName &fileName);

    /*! import a STL file, and add it to the specified model */
    void importSTL(Model &model, const FileName &fileName);

    /*! import a list of STL files */
    void importSTL(std::vector<Model *> &animation, const FileName &fileName);

    /*! import a list of X3D files */
    void importX3D(Model &model, const FileName &fileName);

    /*! import a MiniSG MSG file, and add it to the specified model */
    void importMSG(Model &model, const FileName &fileName);

    void error(const std::string &err);

  } // ::ospray::minisg
} // ::ospray
