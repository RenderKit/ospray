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

#undef NDEBUG

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// sg
#include "SceneGraph.h"
#include "sg/geometry/Spheres.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

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

    // World *createTestAlphaSphereCube(size_t numPerSide)
    // {
    //   sg::World *world = new sg::World;
    //   float radius = .7f/numPerSide;

    //   Ref<sg::AlphaSpheres> spheres = new sg::AlphaSpheres;
    //   Ref<sg::TransferFunction> transferFunction = new sg::TransferFunction;

    //   std::vector<vec3f> position;
    //   sg::AlphaSpheres::Attribute *attribute = new sg::AlphaSpheres::Attribute("test");
      
    //   for (int z=0;z<numPerSide;z++)
    //     for (int y=0;y<numPerSide;y++)
    //       for (int x=0;x<numPerSide;x++) {
    //         vec3f a;
    //         a.x = x/float(numPerSide);
    //         a.y = y/float(numPerSide);
    //         a.z = z/float(numPerSide);
    //         position.push_back(a);

    //         float f = cos(15*a.x*a.y)+sin(12*a.y)+cos(22*a.x+13*a.z)*sin(5*a.z+3*a.x+11*a.y);
            
    //         (*attribute).push_back(f);
    //       }
      
    //   spheres->setRadius(radius);
    //   spheres->setPositions(position);
    //   spheres->addAttribute(attribute);
    //   spheres->setTransferFunction(transferFunction);

    //   world->node.push_back(transferFunction.cast<sg::Node>());
    //   world->node.push_back(spheres.cast<sg::Node>());

    //   return world;
    // }
      
//     World *importCosmicWeb(const char *fileName, size_t maxParticles)
//     {
//       throw std::runtime_error("World *importCosmicWeb(const char *fileName, size_t maxParticles)' not yet ported to new alphaspheres");
// #if 0
//       sg::World *world = new sg::World;
//       sg::AlphaSpheres *spheres = new sg::AlphaSpheres;
      
//       struct Particle {
//         vec3f p,v;
//       } particle;

//       //      maxParticles = std::min(maxParticles,(size_t)2200000); // 25 - 25
//       float radius = 2.f;
//       FILE *file = fopen(fileName,"rb");
//       static float biggestF = 0.f;
//       PING;
//       for (int i=0;i<maxParticles;i++) {
//         int rc = fread(&particle,sizeof(particle),1,file);
//         if (!rc) break;
//         float f = sqrtf(dot(particle.v,particle.v));
//         if (f > biggestF) {
//           biggestF = f;
//           PRINT(biggestF);
//         }
//         // if (f > 200) f = 200;
//         AlphaSpheres::Sphere s(particle.p,radius,f);
//         spheres->sphere.push_back(s);
//       }

//       PING;
//       PRINT(spheres->sphere.size());
//       world->node.push_back(spheres->transferFunction.ptr);
//       world->node.push_back(spheres);
//       PING;
//       return world;
// #endif
//     }
      
  } // ::ospray::sg
} // ::ospray

