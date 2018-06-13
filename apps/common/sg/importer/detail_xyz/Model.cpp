// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "Model.h"
// stl
#include <sstream>

namespace ospray {
  namespace particle {

    float Model::defaultRadius = 1.f;

    inline vec3f makeRandomColor(const int i)
    {
      const int mx = 13*17*43;
      const int my = 11*29;
      const int mz = 7*23*63;
      const uint32_t g = (i * (3*5*127)+12312314);
      return vec3f((g % mx)*(1.f/(mx-1)),
                   (g % my)*(1.f/(my-1)),
                   (g % mz)*(1.f/(mz-1)));
    }

    int Model::getAtomType(const std::string &name)
    {
      auto it = atomTypeByName.find(name);
      if (it == atomTypeByName.end()) {
        std::cout << "Found atom type '"+name+"'" << std::endl;
        AtomType a(name);
        a.color = makeRandomColor(atomType.size());
        int newID = atomType.size();
        atomTypeByName[name] = newID;
        atomType.push_back(a);
        return newID;
      }
      return it->second;
    }

    void Model::loadXYZ(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      if (!file)
        throw std::runtime_error("could not open input file " + fileName);

      int numAtoms;
      int rc = fscanf(file,"%i\n",&numAtoms);
      PRINT(numAtoms);

      if (rc != 1) {
        throw std::runtime_error("could not parse .dat.xyz header in "
                                 "input file " + fileName);
      }

      char line[10000];
      if (!fgets(line,10000,file))
        throw std::runtime_error("could not fgets");

      std::cout << "#" << fileName << " (.dat.xyz format): expecting "
                << numAtoms << " atoms" << std::endl;

      for (int i = 0; i < numAtoms; i++) {
        char atomName[110];
        Atom a;
        vec3f n;
        if (!fgets(line,10000,file)) {
          std::stringstream ss;
          ss << "in " << fileName << " (line " << (i+2) << "): "
             << "unexpected end of file!?" << std::endl;
#if 1
          std::cout << "warning: " << ss.str() << std::endl;
          break;
#endif
        }

        rc = sscanf(line, "%100s %f %f %f %f %f %f\n", atomName,
                    &a.position.x, &a.position.y, &a.position.z,
                    &n.x, &n.y, &n.z);

        if (rc != 7 && rc != 4) {
          std::stringstream ss;
          PRINT(rc);
          PRINT(line);
          ss << "in " << fileName << " (line " << (i+2) << "): "
             << "could not parse .dat.xyz data line" << std::endl;
#if 1
          std::cout << "warning: " << ss.str() << std::endl;
          break;
#endif
        }
        a.type = getAtomType(atomName);
        a.radius = atomType[a.type].radius;
        if (a.radius == 0.f)
          a.radius = defaultRadius;
        atom[a.type].push_back(a);
      }
    }

    /*! load xyz files in which there is *no* atom count, but just a
        list of "type x y z" lines */
    void Model::loadXYZ2(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(), "r");
      if (!file)
        throw std::runtime_error("could not open input file " + fileName);

      int rc = 0;
      char atomTypeName[1000];
      vec3f pos;

      while ((rc = fscanf(file, "%s %f %f %f\n",
                          atomTypeName, &pos.x, &pos.y, &pos.z)) == 4) {
        Atom a = {pos, 0, getAtomType(atomTypeName)};
        atom[a.type].push_back(a);
      }

      if (rc != 4) {
        std::cout << "#" << fileName << " (.xyz format): file may be truncated"
                  << std::endl;
      }
    }

    void Model::loadXYZ3(const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");

      if (!file)
        throw std::runtime_error("could not open input file "+fileName);

      int numAtoms;

      int rc = fscanf(file, "%i\n", &numAtoms);
      PRINT(numAtoms);
      if (rc != 1) {
        throw std::runtime_error("could not parse .dat.xyz header in "
                                 "input file "+fileName);
      }

      char line[10000];
      if (!fgets(line,10000,file))
        throw std::runtime_error("could not fgets");

      std::cout << "#" << fileName << " (.dat.xyz format): expecting "
                << numAtoms << " atoms" << std::endl;

      for (int i=0;i<numAtoms;i++) {
        char atomName[110];
        Atom a;
        vec3f n;
        if (!fgets(line,10000,file)) {
          std::stringstream ss;
          ss << "in " << fileName << " (line " << (i+2) << "): "
             << "unexpected end of file!?" << std::endl;
#if 1
          std::cout << "warning: " << ss.str() << std::endl;
          break;
#endif
        }

        rc = sscanf(line, "%f,%f,%f,%s\n",
                    &a.position.x, &a.position.y, &a.position.z, atomName);

        if (rc != 7 && rc != 4) {
          std::stringstream ss;
          PRINT(rc);
          PRINT(line);
          ss << "in " << fileName << " (line " << (i+2) << "): "
             << "could not parse .dat.xyz data line" << std::endl;
#if 1
          std::cout << "warning: " << ss.str() << std::endl;
          break;
#endif
        }
        a.type = getAtomType(atomName);
        a.radius = atomType[a.type].radius;
        if (a.radius == 0.f)
          a.radius = defaultRadius;
        atom[a.type].push_back(a);
      }
    }

  } // ::ospray::particle
} // ::ospray


