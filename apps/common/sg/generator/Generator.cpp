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

// ospcommon
#include "ospcommon/utility/StringManip.h"
// ospray::sg
#include "Generator.h"
#include "common/sg/SceneGraph.h"
#include "common/sg/geometry/TriangleMesh.h"

#include "../visitor/PrintNodes.h"

/*! \file sg/module/Generator.cpp Defines the interface for writing
  file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    // Generator definitions ///////////////////////////////////////////////////

    Generator::Generator()
    {
      createChild("generatorType", "string");
      createChild("parameters", "string");
    }

    void Generator::generateData()
    {
      auto type   = child("generatorType").valueAs<std::string>();
      auto params = child("parameters").valueAs<std::string>();

      if (lastType.empty() || lastParams.empty()) {
        lastType   = type;
        lastParams = params;
      }

      std::cout << "generating data with '" << lastType << "' generator"
                << std::endl;

      remove("data");
      auto wsg = createChild("data", "Transform").shared_from_this();

      importRegistryGenerator(wsg, lastType, lastParams);
    }

    void Generator::preCommit(RenderContext &/*ctx*/)
    {
      auto type   = child("generatorType").valueAs<std::string>();
      auto params = child("parameters").valueAs<std::string>();

      if (lastType != type || lastParams != params) {
        lastType   = type;
        lastParams = params;
        generateData();
        commit();
      }
    }

    void Generator::importRegistryGenerator(std::shared_ptr<Node> world,
                                            const std::string &type,
                                            const std::string &params) const
    {
      using importFunction = void(*)(std::shared_ptr<Node>,
                                     const std::vector<string_pair> &);

      static std::map<std::string, importFunction> symbolRegistry;

      if (symbolRegistry.count(type) == 0) {
        std::string creationFunctionName = "ospray_sg_generate_" + type;
        symbolRegistry[type] =
            (importFunction)getSymbol(creationFunctionName);
      }

      auto fcn = symbolRegistry[type];

      if (fcn) {
        std::vector<string_pair> parameters;
        auto splitValues = utility::split(params, ',');
        for (auto &value : splitValues) {
          auto splitParam = utility::split(value, '=');
          if (splitParam.size() == 2)
            parameters.emplace_back(splitParam[0], splitParam[1]);
          else if (splitParam.size() == 1)
            parameters.emplace_back(splitParam[0], "");
        }

        fcn(world, parameters);
      } else {
        symbolRegistry.erase(type);
        throw std::runtime_error("Could not find sg generator of type: "
          + type + ".  Make sure you have the correct libraries loaded.");
      }
    }

    OSP_REGISTER_SG_NODE(Generator);

  }// ::ospray::sg
}// ::ospray


