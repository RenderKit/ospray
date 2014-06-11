/*! \file miniSG.h Implements a minimalistic two-level scene graph */
#pragma once

// ospray stuff
#include "ospray/common/ospcommon.h"
#include "ospray/common/managed.h"
// embree stuff
#include "common/sys/filename.h"
// stl stuff
#include <vector>
#include <map>

namespace ospray {
  namespace miniSG {

    struct Texture : public RefCount {
      Texture();

      int channels;
      int depth;
      int width;
      int height;
      char *data;
    };
    
#if 0
    /*! mini 'all-rounder' material that should be able to capture
      the most common material types such as OBJ wavefront */
    struct Material : public RefCount {
      std::string name; /*! symbolic name, if available (can be empty) */
      std::string type; /*! material type (like "OBJ", or "Phong", if
                            available (can be empty) */

      float d;
      float Ns;
      float Ni;
      vec3f Ka; /*!< ambient component */
      vec3f Kd; /*!< diffuse component */
      vec3f Ks; /*!< specular component */
      vec3f Tf; 

      std::string map_d;
      std::string map_Ns;
      std::string map_Ni;
      std::string map_Ka;
      std::string map_Kd;
      std::string map_Ks;
      std::string map_Refl;
      std::string map_Bump;

      Material();
    };
#else
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
          DATA, // iw: don't thikn we should have data as a void pointer - there's no way we could pass that through the api without knowing that kind of type (and size) is behind that pointer ...
          STRING,
        } DataType;

        void clear() {
          assert(this && "Tried to clear a null parameter");
          switch( type ) {
            case STRING:
              if (s) free((void*)s);
              s = NULL;
              break;
            case DATA:
              ptr = NULL;
              break;
            default:
              type = DATA;
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

        void set(void *v) { clear(); type = DATA; ptr = v; }

        void set(float v) { clear(); type = FLOAT; f[0] = v; }
        void set(vec2f v) { clear(); type = FLOAT_2; f[0] = v.x; f[1] = v.y; }
        void set(vec3f v) { clear(); type = FLOAT_3; f[0] = v.x; f[1] = v.y; f[2] = v.z; }
        void set(vec4f v) { clear(); type = FLOAT_3; f[0] = v.x; f[1] = v.y; f[2] = v.z; f[3] = v.w; }

        void set(int32 v) { clear(); type = INT; i[0] = v; }
        void set(vec2i v) { clear(); type = INT_2; i[0] = v.x; i[1] = v.y; }
        void set(vec3i v) { clear(); type = INT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; }
        void set(vec4i v) { clear(); type = INT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; i[3] = v.w; }

        void set(uint32 v) { clear(); type = UINT; i[0] = v; }
        void set(vec2ui v) { clear(); type = UINT_2; i[0] = v.x; i[1] = v.y; }
        void set(vec3ui v) { clear(); type = UINT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; }
        void set(vec4ui v) { clear(); type = UINT_3; i[0] = v.x; i[1] = v.y; i[2] = v.z; i[3] = v.w; }

        Param() {
          s = NULL;
          type = INT;
        }
        union {
          float      f[4];
          int32      i[4];
          uint32     ui[4];
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

      int32 getParam(const char *name, int32 defaultVal);
      vec2i getParam(const char *name, vec2i defaultVal);
      vec3i getParam(const char *name, vec3i defaultVal);
      vec4i getParam(const char *name, vec4i defaultVal);

      uint32 getParam(const char *name, uint32 defaultVal);
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
          Param *p = new Param;
          p->set(v);
          params[name] = p;
        }

      std::string toString() const { return "miniSG::Material"; }

      ParamMap params;
      std::vector<Ref<Texture> > textures;
      std::string name;
      std::string type;
    };
#endif

    struct Triangle {
      uint32 v0, v1, v2;
      //      uint32 materialID; // iw, 1/11/14: disabled materialID
      //      per triangle, emrbee cannot do buffer sharing with this
      //      format.
    };

    /*! default triangle mesh layout */
    struct Mesh : public RefCount {
      std::string           name;     /*!< symbolic name of mesh, can be empty */
      std::vector<vec3fa>   position; /*!< vertex positions */
      std::vector<vec3fa>   normal;   /*!< vertex normals; empty if none present */
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
      std::vector<uint32> triangleMaterialId;
      
      
      box3f bounds; /*!< bounding box of all vertices */

      int size() const { return triangle.size(); }
      Ref<Material> material;
      box3f getBBox();
      Mesh() : bounds(embree::empty) {};
    };

    struct Instance : public RefCount {
      affine3f xfm;
      int meshID;
      
      Instance(int meshID=0, affine3f xfm=embree::one) : meshID(meshID), xfm(xfm) {};
    };
    bool operator==(const Instance &a, const Instance &b);
    bool operator!=(const Instance &a, const Instance &b);

    struct Model : public RefCount {
      // /*! list of materials - if per-triangle material IDs are used,
      //     then all material IDs of all meshes reference into this
      //     list */
      // std::vector<Ref<Material> > material;
      /*! list of meshes that the scene is composed of */
      std::vector<Ref<Mesh> >     mesh;
      /*! \brief list of instances (if available). */
      std::vector<Instance>      instance;

      //! return number of meshes in this model
      inline size_t numMeshes() const { return mesh.size(); }
      //! return number of unique triangles (ie, _ex_ instantiation!) in this model
      size_t numUniqueTriangles() const;
      /*! computes and returns the world-space bounding box of the entire model */
      box3f getBBox();
    };

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importOBJ(Model &model, const embree::FileName &fileName);

    /*! import a TRI file (format:vec3fa[3][numTris]), and add it to the specified model */
    void importTRI(Model &model, const embree::FileName &fileName);

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importRIVL(Model &model, const embree::FileName &fileName);

    /*! import a STL file, and add it to the specified model */
    void importSTL(Model &model, const embree::FileName &fileName);

    /*! import a list of STL files */
    void importSTL(std::vector<Model *> &animation, const embree::FileName &fileName);

    /*! import a MiniSG MSG file, and add it to the specified model */
    void importMSG(Model &model, const embree::FileName &fileName);

    void error(const std::string &err);

  }
}
