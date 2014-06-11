#include "miniSG.h"

namespace ospray {
  namespace miniSG {

    Texture::Texture()
      : channels(0)
      , depth(0)
      , width(0)
      , height(0)
      , data(NULL)
    {}

    Material::Material()
    {
      setParam( "Kd", vec3f(.7f) );
      setParam( "Ks", vec3f(0.f) );
      setParam( "Ka", vec3f(0.f) );
    }

    float Material::getParam(const char *name, float defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::FLOAT && "Param type mismatch" );
          return it->second->f[0];
        }

        return defaultVal;
    }
    
    vec2f Material::getParam(const char *name, vec2f defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::FLOAT_2 && "Param type mismatch" );
          return vec2f(it->second->f[0], it->second->f[1]);
        }

        return defaultVal;
    }

    vec3f Material::getParam(const char *name, vec3f defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::FLOAT_3 && "Param type mismatch" );
          return vec3f( it->second->f[0], it->second->f[1], it->second->f[2] );
        }

        return defaultVal;
    }

    vec4f Material::getParam(const char *name, vec4f defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::FLOAT_4 && "Param type mismatch" );
          return vec4f( it->second->f[0], it->second->f[1], it->second->f[2], it->second->f[3] );
        }

        return defaultVal;
    }

    int32 Material::getParam(const char *name, int32 defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::INT && "Param type mismatch" );
          return it->second->i[0];
        }

        return defaultVal;
    }

    vec2i Material::getParam(const char *name, vec2i defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::INT_2 && "Param type mismatch" );
          return vec2i(it->second->i[0], it->second->i[1]);
        }

        return defaultVal;
    }

    vec3i Material::getParam(const char *name, vec3i defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::INT_3 && "Param type mismatch" );
          return vec3i(it->second->i[0], it->second->i[1], it->second->i[2]);
        }

        return defaultVal;
    }

    vec4i Material::getParam(const char *name, vec4i defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::INT_4 && "Param type mismatch" );
          return vec4i(it->second->i[0], it->second->i[1], it->second->i[2], it->second->i[3]);
        }

        return defaultVal;
    }


    uint32 Material::getParam(const char *name, uint32 defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::UINT && "Param type mismatch" );
          return it->second->ui[0];
        }

        return defaultVal;
    }

    vec2ui Material::getParam(const char *name, vec2ui defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::UINT_2 && "Param type mismatch" );
          return vec2ui(it->second->ui[0], it->second->ui[1]);
        }

        return defaultVal;
    }

    vec3ui Material::getParam(const char *name, vec3ui defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::UINT_3 && "Param type mismatch" );
          return vec3ui(it->second->ui[0], it->second->ui[1], it->second->ui[2]);
        }

        return defaultVal;
    }

    vec4ui Material::getParam(const char *name, vec4ui defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::UINT_4 && "Param type mismatch" );
          return vec4ui(it->second->ui[0], it->second->ui[1], it->second->ui[2], it->second->ui[3]);
        }

        return defaultVal;
    }


    const char *Material::getParam(const char *name, const char *defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::STRING && "Param type mismatch" );
          return it->second->s;
        }

        return defaultVal;
    }
    void *Material::getParam(const char *name, void *defaultVal) {
        ParamMap::iterator it = params.find(name);
        if (it != params.end()) {
          assert( it->second->type == Param::DATA && "Param type mismatch" );
          return it->second->ptr;
        }

        return defaultVal;
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
