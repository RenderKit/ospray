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

// ospray::sg
#include "../common/Model.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Generator : public sg::Renderable
    {
      Generator();

      void preCommit(RenderContext &ctx) override;

      void generateData();

    private:

      void importRegistryGenerator(std::shared_ptr<Node> world,
                                   const std::string &type,
                                   const std::string &params) const;

      // Data //

      std::string lastType;
      std::string lastParams;
    };

    /*! prototype for any scene graph importer function */
    using GeneratorFunction = void (*)(std::shared_ptr<Node> world,
                                      const FileName &fileName);

    // Macro to register importers ////////////////////////////////////////////

    using string_pair = std::pair<std::string, std::string>;

#define OSPSG_REGISTER_GENERATE_FUNCTION(function, name)                       \
    extern "C" OSPRAY_DLLEXPORT void ospray_sg_generate_##name(                \
      std::shared_ptr<Node> world,                                             \
      const std::vector<string_pair> &params)                                  \
    {                                                                          \
      function(world, params);                                                 \
    }                                                                          \
    /* additional declaration to avoid "extra ;" -Wpedantic warnings */        \
    void ospray_sg_import_##name()

  } // ::ospray::sg
} // ::ospray
