// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

    OSPData makeMaterials(OSPRenderer renderer, const particle::Model &model)
    {
      int numMaterials = model.atomType.size();
      std::vector<OSPMaterial> matArray(numMaterials);

      for (int i = 0; i < numMaterials; i++) {
        OSPMaterial mat = ospNewMaterial(renderer, "OBJMaterial");
        ospSet3fv(mat, "kd", &model.atomType[i]->color.x);
        ospCommit(mat);
        matArray[i] = mat;
      }

      OSPData data = ospNewData(numMaterials,OSP_OBJECT,matArray.data());
      return data;
    }

    void importXYZ(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      particle::Model m;

      if (fileName.ext() == "xyz")
        m.loadXYZ(fileName);
      else if (fileName.ext() == "xyz2")
        m.loadXYZ2(fileName);
      else if (fileName.ext() == "xyz3")
        m.loadXYZ3(fileName);

      auto name = fileName.str();

      for (int i = 0; i < m.atomType.size(); i++) {
        auto spGeom = createNode(name + "_spheres_type_" + std::to_string(i),
                                 "Spheres")->nodeAs<Spheres>();

        spGeom->createChild("bytes_per_sphere", "int",
                            int(sizeof(particle::Model::Atom)));

        spGeom->createChild("offset_center", "int", int(0));
        spGeom->createChild("offset_radius", "int", int(3*sizeof(float)));
        spGeom->createChild("offset_materialID", "int", int(4*sizeof(float)));

        auto spheres =
          std::make_shared<DataVectorT<particle::Model::Atom, OSP_RAW>>();
        spheres->setName("spheres");
        spheres->v = std::move(m.atom[i]);

        spGeom->add(spheres);

        auto &material = spGeom->child("material");

        material["d"].setValue(1.f);
        material["Ka"].setValue(vec3f(0.0f, 0.0f, 0.0f));
        material["Kd"].setValue(m.atomType[i]->color);
        material["Ks"].setValue(vec3f(0.2f, 0.2f, 0.2f));

        auto model = createNode(name + "_spheres_model", "Model");
        model->add(spGeom);

        auto instance = createNode(name + "_spheres_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
      }
    }

  } // ::ospray::sg
} // ::ospray


