#undef NDEBUG

#include "model.h"

namespace ospray {
  namespace tachyon {

    using std::cout;
    using std::endl;

    const char *TOK = "\t\n ";
    const int LINESZ=10000;

    inline bool operator<(const Texture &a, const Texture &b)
    {
      int v = memcmp(&a,&b,sizeof(a));
      return v < 0;
    }
    inline bool operator==(const Texture &a, const Texture &b)
    {
      int v = memcmp(&a,&b,sizeof(a));
      return v == 0;
    }

    Texture::Texture()
      : ambient(0), diffuse(.8), specular(0), opacity(1)
    {
    }
    Texture::~Texture()
    {
    }

    Phong::Phong()
      : plastic(0), size(0), color(1,1,1), texFunc(0)
    {}

    Camera::Camera()
      : center(0,0,0),
        upDir(0,1,0),
        viewDir(0,0,1)
    {
    }

    Model::Model()
      : bounds(embree::empty),
        camera(NULL),
        smoothTrisVA(NULL),
        resolution(-1)
    {
    }

    box3f Model::getBounds() const { return bounds; }

    bool Model::empty() const { return bounds.empty(); }

    VertexArray *Model::getSTriVA(bool create) {
      if (!smoothTrisVA && create) {
        smoothTrisVA = new VertexArray();
        vertexArray.push_back(smoothTrisVA);
      }
      return smoothTrisVA;
    }

    void Model::addTriangle(const Triangle &triangle)
    {
      this->triangle.push_back(triangle);
      bounds.extend(triangle.v0);
      bounds.extend(triangle.v1);
      bounds.extend(triangle.v2);
    }
    void Model::addVertexArray(VertexArray *va)
    {
      this->vertexArray.push_back(va);
      for (int i=0;i<va->coord.size();i++)
        bounds.extend(va->coord[i]);
    }
    void Model::addSphere(const Sphere &sphere)
    {
      this->sphere.push_back(sphere);
      bounds.extend(sphere.center - sphere.rad);
      bounds.extend(sphere.center + sphere.rad);
    }
    void Model::addCylinder(const Cylinder &cylinder)
    {
      this->cylinder.push_back(cylinder);
      bounds.extend(cylinder.base + cylinder.rad);
      bounds.extend(cylinder.base - cylinder.rad);
      bounds.extend(cylinder.apex + cylinder.rad);
      bounds.extend(cylinder.apex - cylinder.rad);
    }
    int Model::addTexture(Texture *texture)
    {
      assert(texture);
      for (int i=allTextures.size()-1;i>=0;--i)
        if (*texture == allTextures[i])
          return i;
      allTextures.push_back(*texture);
      return allTextures.size()-1;
    }

  }
}
