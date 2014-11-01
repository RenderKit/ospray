/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// header
#include "SceneGraph.h"
// stl
#include <map>
// // libxml
#include "apps/common/xml/xml.h"
// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace ospray {
  namespace sg {
    using std::string;
    using std::cout;
    using std::endl;

    World *importSpheres(const std::string &fileName)
    {
      return NULL;
    }

    World *createTestSphere()
    {
      sg::World *world = new sg::World;
      sg::Spheres *spheres = new sg::Spheres;
      spheres->sphere.push_back(sg::Spheres::Sphere(vec3f(0,0,0),1.f));
      // sg::TransferFunction 
      //   = new sg::TransferFunction(sg::TransferFunction::CoolToWarm));
      // spheres->addParam(new NodeParam("transferFunction",transferFunction));
      // world->geometry.push_back(spheres);
      world->node.push_back(spheres);
      return world;
    }
      
    World *createTestSphereCube(size_t numPerSide)
    {
      sg::World *world = new sg::World;
      sg::Spheres *spheres = new sg::Spheres;

      float radius = 1.f/numPerSide;
      for (int z=0;z<numPerSide;z++)
        for (int y=0;y<numPerSide;y++)
          for (int x=0;x<numPerSide;x++) {
            vec3f a;
            a.x = x/float(numPerSide);
            a.y = y/float(numPerSide);
            a.z = z/float(numPerSide);
            Spheres::Sphere s(a,radius,0);
            spheres->sphere.push_back(s);
          }

      world->node.push_back(spheres);
      return world;
    }

    World *createTestAlphaSphereCube(size_t numPerSide)
    {
      sg::World *world = new sg::World;
      sg::AlphaSpheres *spheres = new sg::AlphaSpheres;

      float radius = .7f/numPerSide;
      for (int z=0;z<numPerSide;z++)
        for (int y=0;y<numPerSide;y++)
          for (int x=0;x<numPerSide;x++) {
            vec3f a;
            a.x = x/float(numPerSide);
            a.y = y/float(numPerSide);
            a.z = z/float(numPerSide);
            float f = cos(15*a.x*a.y)+sin(12*a.y)+cos(22*a.x+13*a.z)*sin(5*a.z+3*a.x+11*a.y);
            AlphaSpheres::Sphere s(a,radius,f);
            spheres->sphere.push_back(s);
          }
      
      // for (int i=0;i<spheres->sphere.size();i++) {
      //   AlphaSpheres::Sphere s = spheres->sphere[i];
      //   cout << "sphere " << i << " : " << s.position << " rad " << s.radius << endl;
      // }

      world->node.push_back(spheres->transferFunction.ptr);
      world->node.push_back(spheres);

      return world;
    }
      
    World *importCosmicWeb(const char *fileName, size_t maxParticles)
    {
      sg::World *world = new sg::World;
      sg::AlphaSpheres *spheres = new sg::AlphaSpheres;
      
      struct Particle {
        vec3f p,v;
      } particle;

      //      maxParticles = std::min(maxParticles,(size_t)2200000); // 25 - 25
      float radius = 2.f;
      FILE *file = fopen(fileName,"rb");
      static float biggestF = 0.f;
      PING;
      for (int i=0;i<maxParticles;i++) {
        int rc = fread(&particle,sizeof(particle),1,file);
        if (!rc) break;
        float f = sqrtf(dot(particle.v,particle.v));
        if (f > biggestF) {
          biggestF = f;
          PRINT(biggestF);
        }
        // if (f > 200) f = 200;
        AlphaSpheres::Sphere s(particle.p,radius,f);
        spheres->sphere.push_back(s);
      }

      PING;
      PRINT(spheres->sphere.size());
      world->node.push_back(spheres->transferFunction.ptr);
      world->node.push_back(spheres);
      PING;
      return world;
    }
      
  } // ::ospray::sg
} // ::ospray

