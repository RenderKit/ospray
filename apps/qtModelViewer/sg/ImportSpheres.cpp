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
            float f = cos(a.x)+2*sin(a.y)+cos(2*a.x+a.z)*sin(a.z);
            AlphaSpheres::Sphere s(a,radius,f);
            spheres->sphere.push_back(s);
          }

      world->node.push_back(spheres->transferFunction.ptr);
      world->node.push_back(spheres);
      return world;
    }
      
  } // ::ospray::sg
} // ::ospray

