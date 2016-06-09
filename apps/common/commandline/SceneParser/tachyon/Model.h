// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

// ospray
#include "common/vec.h"
#include "common/box.h"
// std
#include <map>
#include <vector>

namespace ospray {
  namespace tachyon {

    using namespace ospcommon;

    struct Phong {
      float plastic;
      float size;
        // Phong Plastic 0.5 Phong_size 40 Color 1 0 0 TexFunc 0
      Phong();
    };
    struct Texture {
      float ambient, diffuse, specular, opacity;
      Phong phong;
      vec3f color;
      int texFunc;
      Texture();
      ~Texture();
    };
    struct Camera {
      vec3f center;
      vec3f upDir;
      vec3f viewDir;
      Camera();
    };
    struct PointLight {
      vec3f center;
      vec3f color;
      struct Attenuation {
        float constant, linear, quadratic;
      } atten;
    };
    struct DirLight {
      vec3f color;
      vec3f direction;
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
      std::vector<vec3fa> coord;
      std::vector<vec3fa> color;
      std::vector<vec3fa> normal;
      std::vector<vec3i>  triangle;
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
      void addPointLight(const PointLight &pointLight);
      void addDirLight(const DirLight &dirLight);

      const Texture *getTexture(size_t ID) const { return &textureVec[ID]; }
      VertexArray *getVertexArray(int i)   const { return vertexArrayVec[i]; }
      Camera *getCamera(bool create=false) 
      { if (camera == NULL && create) camera = new Camera; return camera; }
      VertexArray *getSTriVA(bool create=false);

      const void  *getTrianglesPtr()       const { return &triangleVec[0]; }
      const void  *getSpheresPtr()         const { return &sphereVec[0]; }
      const void  *getPointLightsPtr()     const { return &pointLightVec[0]; }
      const void  *getDirLightsPtr()       const { return &dirLightVec[0]; }
      const void  *getCylindersPtr()       const { return &cylinderVec[0]; }
      const void  *getTexturesPtr()        const { return &textureVec[0]; }

      size_t numCylinders()    const { return cylinderVec.size(); };
      size_t numSpheres()      const { return sphereVec.size(); };
      size_t numPointLights()  const { return pointLightVec.size(); };
      size_t numDirLights()    const { return dirLightVec.size(); };
      size_t numTriangles()    const { return triangleVec.size(); };
      size_t numTextures()     const { return textureVec.size(); };
      size_t numVertexArrays() const { return vertexArrayVec.size(); };

      void exportToEmbree(const std::string &fileName);
      
    private:
      box3f bounds;
      std::vector<Triangle>      triangleVec;
      std::vector<Cylinder>      cylinderVec;
      std::vector<Sphere>        sphereVec;
      std::vector<VertexArray *> vertexArrayVec;
      std::vector<DirLight>      dirLightVec;
      std::vector<PointLight>    pointLightVec;
      std::vector<Texture>       textureVec;

      Camera *camera;

      /*! "STri" (smooth triangle) constructs have vertex normals, but
          to feed them into embree we first have to convert them to
          vertex arrays, for which purpose we store them into this
          VA */
      VertexArray *smoothTrisVA;

    };

    void importFile(tachyon::Model &model, const std::string &fileName);

  } // ::ospray::tachyon
} // ::ospray
