/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */
#pragma once

// ospray
#include "common/ospcommon.h"
// stl
#include <map>
#include <vector>

namespace ospray {
  namespace particle {

    struct Model {
      struct AtomType {
        const std::string name;
        vec3f             color;

        AtomType(const std::string &name) : name(name), color(.7f) {};
      };
      struct Atom {
        vec3f position;
        int type; /*! type of the model in atomType - also serves as a material ID */
      };

      std::vector<AtomType *>   atomType;
      std::map<std::string,int> atomTypeByName;
      std::vector<Atom> atom;
      float radius;

      int getAtomType(const std::string &name);

      /*! \brief load lammps xyz files */
      void loadXYZ(const std::string &fn);
      
      box3f getBBox() const;

      Model() : radius(1.f) {}
    };

  }
}
