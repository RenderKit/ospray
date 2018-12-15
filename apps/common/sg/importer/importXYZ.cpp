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

#undef NDEBUG

// sg
#include "SceneGraph.h"
#include "sg/geometry/Spheres.h"

#include "detail_xyz/Model.h"

namespace ospray {
  namespace sg {

    void importXYZ(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      particle::Model m;

      if (fileName.ext() == "xyz")
        m.loadXYZ(fileName);
      else if (fileName.ext() == "xyz2")
        m.loadXYZ2(fileName);
      else if (fileName.ext() == "xyz3")
        m.loadXYZ3(fileName);

      for (size_t i = 0; i < m.atomType.size(); i++) {
        auto name = fileName.str() + "_type_" + std::to_string(i);

        auto spGeom = createNode(name + "_geometry",
                                 "Spheres")->nodeAs<Spheres>();

        spGeom->createChild("bytes_per_sphere", "int",
                            int(sizeof(particle::Model::Atom)));
        spGeom->createChild("offset_center", "int", int(0));
        spGeom->createChild("offset_radius", "int", int(3*sizeof(float)));

        auto spheres =
          std::make_shared<DataVectorT<particle::Model::Atom, OSP_RAW>>();
        spheres->setName("spheres");
        auto atomTypeID = m.atomTypeByName[m.atomType[i].name];
        spheres->v = std::move(m.atom[atomTypeID]);

        spGeom->add(spheres);

        auto materials = spGeom->child("materialList").nodeAs<MaterialList>();
        materials->item(0)["d"]  = 1.f;
        materials->item(0)["Kd"] = m.atomType[i].color;
        materials->item(0)["Ks"] = vec3f(0.2f);

        auto model = createNode(name + "_model", "Model");
        model->add(spGeom);

        auto instance = createNode(name + "_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
      }
    }

  } // ::ospray::sg
} // ::ospray


