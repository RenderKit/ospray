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

// own
#include "Importer.h"
// ospcommon
#include "ospcommon/FileName.h"

// scene graph
#include "sg/module/Module.h"
#include "sg/importer/Importer.h"
#include "sg/Renderer.h"

#include <string>

namespace ospray {
  namespace importer {

    void importOSP(const FileName &fileName, Group *existingGroupToAddTo);
    void importRM(const FileName &fileName, Group *existingGroupToAddTo);

    Group *import(const std::string &fn, Group *existingGroupToAddTo)
    {
      FileName fileName = fn;
      Group *group = existingGroupToAddTo;
      if (!group) group = new Group;

      if (fileName.ext() == "osp") {
        importOSP(fn, group);
#if 0 // NOTE(jda) - this can only be re-enabled once the importer stuff is off of Ref<>
//#ifndef _WIN32
      } else if (fileName.ext() == "osg") {
          Ref<sg::World> world = new sg::World;
          world = sg::loadOSG(fn);
          Ref<sg::Volume> volumeNode;
          for (auto node : world.ptr->node)
          {
            std::cout << "found node: " << node.ptr->toString() << std::endl;
            if (node->toString().find("Chombo") != std::string::npos)
              volumeNode = Ref<sg::Volume>((sg::Volume*)node.ptr);
          }
          if (!volumeNode)
          {
            throw std::runtime_error("#ospray:importer: no volume found in osg file");
          }
          sg::RenderContext ctx;
          Ref<sg::Integrator>  integrator;
          integrator = new sg::Integrator("scivis");
          ctx.integrator = integrator.ptr;
          integrator->commit();
          assert(ctx.world);
          if (!world) {
            std::cout << "#osp:qtv: no world defined. exiting." << std::endl;
            exit(1);
          }

          world->render(ctx);
          assert(world->ospModel);

          OSPVolume volume = volumeNode->volume;
          assert(volume);

          Volume* msgVolume = new Volume;
          msgVolume->bounds = volumeNode->getBounds();
          msgVolume->handle = volumeNode->volume;
          group->volume.push_back(msgVolume);
#endif
      } else if (fileName.ext() == "bob") {
        importRM(fn, group);
      } else {
        throw std::runtime_error("#ospray:importer: do not know how to import file of type "
            + fileName.ext());
      }

      return group;
    }

  }
}
