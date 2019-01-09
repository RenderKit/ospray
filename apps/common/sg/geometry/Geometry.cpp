// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Geometry.h"
#include "../common/Model.h"

namespace ospray {
  namespace sg {

    Geometry::Geometry(const std::string &type)
    {
      auto matList =
        createChild("materialList", "MaterialList").nodeAs<MaterialList>();
      matList->push_back(createNode("default", "Material")->nodeAs<Material>());

      createChild("type", "string", type);
      setValue((OSPGeometry)nullptr);
    }

    std::string Geometry::toString() const
    {
      return "ospray::sg::Geometry";
    }

    void Geometry::preCommit(RenderContext &)
    {
      auto ospGeometry = valueAs<OSPGeometry>();
      if (!ospGeometry) {
        auto type = child("type").valueAs<std::string>();
        ospGeometry = ospNewGeometry(type.c_str());
        setValue(ospGeometry);

        child("bounds") = computeBounds();
      }
    }

    void Geometry::postCommit(RenderContext &)
    {
      auto ospGeometry = valueAs<OSPGeometry>();

      if (hasChild("material") && !hasChild("materialList")) {
        // XXX FIXME never happens
        ospSetMaterial(ospGeometry, child("material").valueAs<OSPMaterial>());
      }

      auto materialListNode = child("materialList").nodeAs<MaterialList>();
      const auto &materialList = materialListNode->nodes;
      if (!materialList.empty()) {
        std::vector<OSPObject> mats;
        for (auto mat : materialList) {
          auto m = mat->valueAs<OSPObject>();
          if (m)
            mats.push_back(m);
        }
        auto ospMaterialList = ospNewData(mats.size(), OSP_OBJECT, mats.data());
        ospCommit(ospMaterialList);
        ospSetData(valueAs<OSPObject>(), "materialList", ospMaterialList);
        ospRelease(ospMaterialList);
      }

      ospCommit(ospGeometry);
    }

    void Geometry::postRender(RenderContext& ctx)
    {
      auto ospGeometry = valueAs<OSPGeometry>();
      if (ospGeometry)
        ospAddGeometry(ctx.currentOSPModel, ospGeometry);
    }

  } // ::ospray::sg
} // ::ospray
