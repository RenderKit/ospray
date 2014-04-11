/*! \file miniSG.h Implements a minimalistic two-level scene graph */
#pragma once

// ospray stuff
#include "ospray/common/ospcommon.h"
// embree stuff
#include "common/sys/filename.h"
// stl stuff
#include <vector>

namespace ospray {
  namespace miniSG {
    
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
      /*! list of materials - if per-triangle material IDs are used,
          then all material IDs of all meshes reference into this
          list */
      std::vector<Ref<Material> > material;
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
