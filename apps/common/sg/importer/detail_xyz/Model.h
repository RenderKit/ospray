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

#pragma once

// ospcommon
#include "ospcommon/FileName.h"
#include "ospcommon/box.h"
#include "ospcommon/containers/AlignedVector.h"
// stl
#include <map>

namespace ospray {
  namespace particle {
    using namespace ospcommon;

    struct Model
    {
      struct AtomType
      {
        const std::string name;
        vec3f             color {0.7f};
        float             radius {0.f};

        AtomType(const std::string &name) : name(name) {}
      };

      struct Atom
      {
        vec3f position;
        float radius;

        /*! type of the model in atomType - also serves as a material ID */
        int type;
      };

      containers::AlignedVector<AtomType> atomType;
      std::map<std::string, int> atomTypeByName;
      std::map<int, containers::AlignedVector<Atom>> atom; //NOTE(jda) - maps typeID->atoms

      //! list of attribute values, if applicable.
      std::map<std::string, containers::AlignedVector<float>*> attribute;

      int getAtomType(const std::string &name);

      /*! \brief load lammps xyz files */
      void loadXYZ(const std::string &fn);
      /*! load xyz files in which there is *no* atom count, but just a
        list of "type x y z" lines */
      void loadXYZ2(const std::string &fileName);
      void loadXYZ3(const std::string &fileName);

      static float defaultRadius;

      void addAttribute(const std::string &name, float value)
      {
        if (!attribute[name])
          attribute[name] = new containers::AlignedVector<float>();
        attribute[name]->push_back(value);
      }
    };

  } // ::ospray::particle
} // ::ospray
