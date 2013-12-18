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
      vec3f kd; /*!< diffuse component */
      vec3f ks; /*!< specular component */

      Material();
    };

    struct Triangle {
      uint v0, v1, v2;
      uint materialID;
    };

    /*! default triangle mesh layout */
    struct Mesh : public RefCount {
      std::string           name;     /*!< symbolic name of mesh, can be empty */
      std::vector<vec3fa>   position; /*!< vertex positions */
      std::vector<vec3fa>   normal;   /*!< vertex normals; empty if none present */
      std::vector<vec2f>    texcoord; /*!< vertex texcoords; empty if none present */
      std::vector<Triangle> triangle; /*!< triangles' vertex IDs */

      int size() const { return triangle.size(); }
    };

    struct Instance : public RefCount {
      affine3f xfm;
      int meshID;
      
      Instance(int meshID=0, affine3f xfm=embree::one) : meshID(meshID), xfm(xfm) {};
    };

    struct Model : public RefCount {
      /*! list of materials - all material IDs of all meshes reference into this list */
      std::vector<Ref<Material> > material;
      /*! list of meshes that the scene is composed of */
      std::vector<Ref<Mesh> >     mesh;
      /*! \brief list of instances (if available). */
      std::vector<Instance>      instance;
    };

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importOBJ(Model &model, const embree::FileName &fileName);

    /*! import a STL file, and add it to the specified model */
    void importSTL(Model &model, const embree::FileName &fileName);

    /*! import a MiniSG MSG file, and add it to the specified model */
    void importMSG(Model &model, const embree::FileName &fileName);

    void error(const std::string &err);
  }
}
