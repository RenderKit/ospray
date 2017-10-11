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

#include "../common/World.h"

namespace ospray {
  namespace sg {

    struct ImportedModel : public sg::Model
    {
      ImportedModel();
      virtual ~ImportedModel() override = default;
      virtual std::string toString() const override;

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;

      // Data //

      ospcommon::box3f bbox;
    };

    ImportedModel::ImportedModel()
    {
      setValue((OSPObject)nullptr);
    }

    std::string ImportedModel::toString() const
    {
      return "ospray::sg::ImportedModel";
    }

    void ImportedModel::preCommit(RenderContext &ctx)
    {
      stashedModel = ctx.currentOSPModel;
      ctx.currentOSPModel = ospModel();
    }

    void ImportedModel::postCommit(RenderContext &ctx)
    {
      ctx.currentOSPModel = stashedModel;
    }

    OSP_REGISTER_SG_NODE(ImportedModel);

    void importOSPModel(Node &world, OSPModel model,
                        const ospcommon::box3f &bbox)
    {
      ospCommit(model);

      auto instanceNode = createNode("appModel_instance", "Instance");

      auto modelNodePtr = createNode("appModel", "ImportedModel");
      auto &modelNode = *modelNodePtr;

      modelNode = (OSPObject)model;
      modelNode["bounds"] = bbox;

      instanceNode->setChild("model", modelNodePtr);
      modelNode.setParent(instanceNode);

      world.add(instanceNode);
    }

  } // ::ospray::sg
} // ::ospray