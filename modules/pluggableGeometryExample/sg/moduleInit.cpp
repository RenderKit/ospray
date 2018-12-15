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

/*! \file ospray/moduleInit \brief Defines the module initialization callback */

#include "sg/module/Module.h"
#include "sg/importer/Importer.h"

#include <stdexcept>
#include <vector>

#include "ospcommon/vec.h"
#include "ospcommon/box.h"

#include "common/sg/common/Data.h"
#include "common/sg/geometry/Geometry.h"
#include "common/sg/importer/Importer.h"

namespace ospray {
  namespace sg {

    /*! though not required, it is good practice to put any module into
      its own namespace (isnide of ospray:: ). Unlike for the naming of
      library and init function, the naming for this namespace doesn't
      particularlly matter. E.g., 'bilinearPatch', 'module_blp',
      'bilinar_patch' etc would all work equally well. */
    namespace bilinearPatch {

      // use ospcommon for vec3f etc
      using namespace ospcommon;

      struct Patch
      {
        Patch(const vec3f &v00,
              const vec3f &v01,
              const vec3f &v10,
              const vec3f &v11)
          : v00(v00), v01(v01), v10(v10), v11(v11)
        {}

        vec3f v00, v01, v10, v11;
      };

      struct PatchSGNode : public sg::Geometry
      {
        PatchSGNode() : Geometry("bilinear_patches") {}

        std::string toString() const override
        {
          return "ospray::bilinearPatch::PatchSGNode";
        }

        box3f bounds() const override
        {
          box3f bounds = empty;
          if (hasChild("vertices")) {
            auto v = child("vertices").nodeAs<sg::DataBuffer>();
            for (uint32_t i = 0; i < v->size(); i++)
              bounds.extend(v->get<vec3f>(i));
          }
          return bounds;
        }
      };

      /*! parse a '.patch' file, and add its contents to the given list of
        patches */
      void readPatchesFromFile(std::vector<Patch> &patches,
                               const std::string &patchFileName)
      {
        FILE *file = fopen(patchFileName.c_str(),"r");
        if (!file)
          throw std::runtime_error("could not open input file '"+patchFileName+"'");

        std::vector<vec3f> parsedPoints;

        size_t numPatchesRead = 0;
        static const size_t lineSize = 10000;
        char line[lineSize];
        while (fgets(line,10000,file)) {
          // try to parse three floats...
          vec3f p;
          int rc = sscanf(line,"%f %f %f",&p.x,&p.y,&p.z);
          if (rc != 3)
            // could not read a point - must be a empty or comment line; just ignore
            continue;

          // add this point to list of parsed points ...
          parsedPoints.push_back(p);

          // ... and if we have four of them, we have a patch!
          if (parsedPoints.size() == 4) {
            patches.push_back(Patch(parsedPoints[0],parsedPoints[1],
                                    parsedPoints[2],parsedPoints[3]));
            parsedPoints.clear();
            ++numPatchesRead;
          }
        }
      }

      void importPatches(const std::shared_ptr<Node> &world,
                         const FileName &fileName)
      {
        std::vector<Patch> patches;
        readPatchesFromFile(patches, fileName);

        if (patches.empty())
          return;

        auto &instance = world->createChild("patches_instance", "Instance");

        auto patchesGeometryNode = std::make_shared<PatchSGNode>();
        patchesGeometryNode->setName("loaded_example_patches");
        patchesGeometryNode->setType("PatchSGNode");

        auto patchArrayNode = std::make_shared<sg::DataVector3f>();

        for (auto &p : patches) {
          patchArrayNode->push_back(p.v00);
          patchArrayNode->push_back(p.v01);
          patchArrayNode->push_back(p.v10);
          patchArrayNode->push_back(p.v11);
        }

        patchArrayNode->setName("vertices");
        patchArrayNode->setType("DataVector3f");
        patchesGeometryNode->add(patchArrayNode);
        instance["model"].add(patchesGeometryNode);
      }

      OSPSG_REGISTER_IMPORT_FUNCTION(importPatches, patches);

      /*! module registry function that initalizes this module with the
        scene graph - in our case, we register a importer for '*.patches'
        files */
      extern "C" void ospray_sg_bilinear_patches_init()
      {
        ospLoadModule("bilinear_patches");
        ospray::sg::declareImporterForFileExtension("patches",importPatches);
      }

    } // ::ospray::sg::bilinear
  } // ::ospray::sg
} // ::ospray
