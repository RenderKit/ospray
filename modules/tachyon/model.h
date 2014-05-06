#pragma once

#include "ospray/common/ospcommon.h"
#include <map>
#include <vector>

namespace ospray {
  namespace tachyon {
    struct Phong {
      float plastic;
      float size;
      vec3f color;
      int texFunc;
        // Phong Plastic 0.5 Phong_size 40 Color 1 0 0 TexFunc 0
      Phong();
    };
    struct Texture {
      float ambient, diffuse, specular, opacity;
      Phong phong;
      Texture();
      ~Texture();
    };
    struct Camera {
      vec3f center;
      vec3f upDir;
      vec3f viewDir;
      Camera();
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
    struct VertexArray : public Primitive {
      std::vector<ospray::vec3fa> coord;
      std::vector<ospray::vec3fa> color;
      std::vector<ospray::vec3fa> normal;
      std::vector<ospray::vec3i>  triangle; 
      /*! for smoothtris (which we put into a vertex array), each
          primitive may have a different texture ID. since 'regular'
          vertex arrays don't, this array _may_ be empty */
      std::vector<int>            perTriTextureID;
    };

    struct Model {
      Model();

      /*! returns if the model is empty ... */
      bool empty() const;
      /*! the current bounding box ... */
      box3f getBounds() const;

      vec2i resolution;
      vec3f backgroundColor;

      int addTexture(Texture *texture);
      void addTriangle(const Triangle &triangle);
      void addSphere(const Sphere &sphere);
      void addCylinder(const Cylinder &cylinder);
      void addVertexArray(VertexArray *va);
      size_t numCylinders() const { return cylinder.size(); };
      size_t numSpheres()   const { return sphere.size(); };
      size_t numTriangles() const { return triangle.size(); };
      size_t numTextures()  const { return allTextures.size(); };
      size_t numVertexArrays() const { return vertexArray.size(); };
      const void  *getTrianglePtr() const { return &triangle[0]; }
      const void  *getSpherePtr()   const { return &sphere[0]; }
      const void  *getCylinderPtr() const { return &cylinder[0]; }
      const void  *getTexturesPtr() const { return &allTextures[0]; }
      VertexArray *getVertexArray(int i) const { return vertexArray[i]; }
      Camera *getCamera(bool create=false) { if (camera == NULL && create) camera = new Camera; return camera; }
      VertexArray *getSTriVA(bool create=false);
    private:
      box3f bounds;
      std::vector<Triangle> triangle;
      std::vector<Cylinder> cylinder;
      std::vector<Sphere>   sphere;
      std::vector<VertexArray *> vertexArray;
      Camera *camera;

      /*! "STri" (smooth triangle) constructs have vertex normals, but
          to feed them into embree we first have to convert them to
          vertex arrays, for which purpose we store them into this
          VA */
      VertexArray *smoothTrisVA;

      std::vector<Texture> allTextures;
    };

    void importFile(tachyon::Model &model, const std::string &fileName);
    void error(const std::string &vol);

  }
}
