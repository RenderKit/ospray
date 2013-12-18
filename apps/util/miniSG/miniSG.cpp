#include "miniSG.h"

namespace ospray {
  namespace miniSG {
    Material::Material()
      : kd(vec3f(.7f)),
        ks(vec3f(0.f))
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

  }
}
