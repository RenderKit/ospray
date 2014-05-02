#include "ospray/common/ospcommon.h"

namespace ospray {
  namespace tachyon {
    struct Phong {
      float plastic;
      float size;
      vec3f color;
      int texFunc;
        // Phong Plastic 0.5 Phong_size 40 Color 1 0 0 TexFunc 0
    };
    
    struct Texture {
      float ambient, diffuse, specular, opacity;
      Phong phong;
    };
    struct Primitive {
      int textureID;
    };
    struct Sphere : public Primitive {
      vec3f center;
      float rad;
    };
    struct Cylinder : public Primitive  {
      vec3f base, apex;
      float rad;
    };
    struct Triangle : public Primitive  {
      vec3f v0,v1,v2;
      vec3f n0,n1,n2;
    };

    struct Model {
      Model();

      /*! returns if the model is empty ... */
      bool empty() const;
      /*! the current bounding box ... */
      box3f getBounds() const;

      vec2i resolution;
      
      int addTexture(const Texture &texture);
      void addTriangle(const Triangle &triangle);
      void addSphere(const Sphere &sphere);
      void addCylinder(const Cylinder &cylinder);
      size_t numCylinders() const { return cylinder.size(); };
      size_t numSpheres()   const { return sphere.size(); };
      size_t numTriangles() const { return triangle.size(); };
      const void *getTrianglePtr() const { return &triangle[0]; }
      const void *getSpherePtr()   const { return &sphere[0]; }
      const void *getCylinderPtr() const { return &cylinder[0]; }
    private:
      box3f bounds;
      std::vector<Triangle> triangle;
      std::vector<Cylinder> cylinder;
      std::vector<Sphere>   sphere;
    };

    void importFile(tachyon::Model &model, const std::string &fileName);
    void error(const std::string &vol);

  }
}
