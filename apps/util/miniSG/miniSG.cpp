#include "miniSG.h"

namespace ospray {
  namespace miniSG {
    Material::Material()
      : Kd(vec3f(.7f)),
        Ks(vec3f(0.f)),
        Ka(vec3f(0.f))
    {
    }

    void error(const std::string &err)
    {
      throw std::runtime_error("ospray::miniSG fatal error : "+err);
    }

    bool operator==(const Instance &a, const Instance &b)
    { return a.meshID == b.meshID && a.xfm == b.xfm; }
    bool operator!=(const Instance &a, const Instance &b)
    { return !(a==b); }

    
    /*! computes and returns the world-space bounding box of given mesh */
    box3f Mesh::getBBox() 
    {
      if (bounds.empty()) {
        for (int i=0;i<position.size();i++)
          bounds.extend(position[i]);
      }
      return bounds;
    }

    /*! computes and returns the world-space bounding box of the entire model */
    box3f Model::getBBox() 
    {
      // this does not yet properly support instancing with transforms!
      box3f bBox = embree::empty;
      for (int i=0;i<mesh.size();i++)
        bBox.extend(mesh[i]->getBBox());
      PING;
      PRINT(bBox);
      return bBox;
    }

    size_t Model::numUniqueTriangles() const
    {
      size_t sum = 0;
      for (int i=0;i<mesh.size();i++)
        sum += mesh[i]->triangle.size();
      return sum;
    }
  }
}
