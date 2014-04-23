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
      if (!instance.empty()) {
        std::vector<box3f> meshBounds;
        for (int i=0;i<mesh.size();i++)
          meshBounds.push_back(mesh[i]->getBBox());
        for (int i=0;i<instance.size();i++) {
          box3f b_i = meshBounds[instance[i].meshID];
          vec3f corner;
          for (int iz=0;iz<2;iz++) {
            corner.z = iz?b_i.upper.z:b_i.lower.z;
            for (int iy=0;iy<2;iy++) {
              corner.y = iy?b_i.upper.y:b_i.lower.y;
              for (int ix=0;ix<2;ix++) {
                corner.x = ix?b_i.upper.x:b_i.lower.x;
                bBox.extend(xfmPoint(instance[i].xfm,corner));
              }
            }
          }
        }
      } else {
        for (int i=0;i<mesh.size();i++)
          bBox.extend(mesh[i]->getBBox());
      }
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
