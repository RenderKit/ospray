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

#include <vector>

#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"
#include "common/sg/common/Data.h"
#include "common/sg/geometry/Geometry.h"

#include "CommandLine.h"
#include "Patch.h"

#include "exampleViewer/widgets/imguiViewer.h"

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {

  /*! though not required, it is good practice to put any module into
    its own namespace (isnide of ospray:: ). Unlike for the naming of
    library and init function, the naming for this namespace doesn't
    particularlly matter. E.g., 'bilinearPatch', 'module_blp',
    'bilinar_patch' etc would all work equally well. */
  namespace bilinearPatch {

    /*! A Simple Triangle Mesh that stores vertex, normal, texcoord,
        and vertex color in separate arrays */
    struct PatchSGNode : public sg::Geometry
    {
      PatchSGNode() : Geometry("bilinear_patches") {}

      box3f bounds() const override
      {
        box3f bounds = empty;
        if (hasChild("vertex")) {
          auto v = child("vertex").nodeAs<sg::DataBuffer>();
          for (uint32_t i = 0; i < v->size(); i++)
            bounds.extend(v->get<vec3fa>(i));
        }
        return bounds;
      }
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override {}
    };

    // use ospcommon for vec3f etc
    using namespace ospcommon;

    extern "C" int main(int ac, const char **av)
    {
      int init_error = ospInit(&ac, av);
      if (init_error != OSP_NO_ERROR) {
        std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
        return init_error;
      }

      auto device = ospGetCurrentDevice();
      if (device == nullptr) {
        std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
        return 1;
      }

      ospDeviceSetStatusFunc(device, [](const char *msg) { std::cout << msg; });
      ospDeviceSetErrorFunc(device,
                            [](OSPError e, const char *msg) {
                              std::cout << "OSPRAY ERROR [" << e << "]: "
                                        << msg << std::endl;
                              std::exit(1);
                            });

      ospDeviceCommit(device);

      // access/load symbols/sg::Nodes dynamically
      loadLibrary("ospray_sg");
      ospLoadModule("simd_bilinear_patches");

      ospray::imgui3D::init(&ac,av);

      // parse the commandline; complain about anything we do not
      // recognize
      CommandLine args(ac,av);

      // import the patches from the sample files (creates a default
      // patch if no files were specified)
      box3f worldBounds;
      std::vector<Patch> patches =
          readPatchesFromFiles(args.inputFiles,worldBounds);

      auto renderer_ptr = sg::createNode("renderer", "Renderer");
      auto &renderer = *renderer_ptr;

      auto &win_size = ospray::imgui3D::ImGui3DWidget::defaultInitSize;
      renderer["frameBuffer"]["size"] = win_size;

      renderer["rendererType"] = std::string("raycast");

      auto &world = renderer["world"];

      auto &patchesInstance = world.createChild("patches", "Instance");

      auto patchesGeometryNode = std::make_shared<PatchSGNode>();
      patchesGeometryNode->setName("loaded_example_patches");
      patchesGeometryNode->setType("PatchSGNode");

      auto patchArrayNode =
          std::make_shared<sg::DataArray1f>((float*)patches.data(),
                                            patches.size() * 12,
                                            false);
      patchArrayNode->setName("patches");
      patchArrayNode->setType("DataArray1f");
      patchesGeometryNode->add(patchArrayNode);
      patchesInstance["model"].add(patchesGeometryNode);

      ospray::ImGuiViewer window(renderer_ptr);

      auto &viewPort = window.viewPort;
      // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
      auto dir = normalize(viewPort.at - viewPort.from);
      renderer["camera"]["dir"] = dir;
      renderer["camera"]["pos"] = viewPort.from;
      renderer["camera"]["up"]  = viewPort.up;
      renderer["camera"]["fovy"] = viewPort.openingAngle;
      renderer["camera"]["apertureRadius"] = viewPort.apertureRadius;
      if (renderer["camera"].hasChild("focusdistance"))
        renderer["camera"]["focusdistance"] = length(viewPort.at - viewPort.from);

      window.create("OSPRay Example Viewer (module) App");

      ospray::imgui3D::run();
      return 0;
    }

  } // ::ospray::bilinearPatch
} // ::ospray
