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

#include "OSPApp.h"
#include "common/sg/SceneGraph.h"

namespace ospray {
namespace app {

OSPApp::OSPApp() {}

int OSPApp::initializeOSPRay(int argc, const char *argv[]) {
  int init_error = ospInit(&argc, argv);
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
  ospDeviceSetErrorFunc(device, [](OSPError e, const char *msg) {
    std::cout << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
  });

  ospDeviceCommit(device);

  return 0;
}

int OSPApp::parseCommandLine(int &ac, const char **&av) {
  clTransform currentCLTransform;
  bool inAnimation = false;
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-r" || arg == "--renderer") {
      initialRendererType = av[i+1];
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "-m" || arg == "--module") {
      ospLoadModule(av[i+1]);
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "--matrix") {
      matrix_i = atoi(av[i+1]);
      matrix_j = atoi(av[i+2]);
      matrix_k = atoi(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "--hdri-light") {
      hdri_light = av[i+1];
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "--translate") {
      currentCLTransform.translate.x = atof(av[i+1]);
      currentCLTransform.translate.y = atof(av[i+2]);
      currentCLTransform.translate.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "--scale") {
      currentCLTransform.scale.x = atof(av[i+1]);
      currentCLTransform.scale.y = atof(av[i+2]);
      currentCLTransform.scale.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "--rotate") {
      currentCLTransform.rotation.x = atof(av[i+1]);
      currentCLTransform.rotation.y = atof(av[i+2]);
      currentCLTransform.rotation.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "--animation") {
      inAnimation = true;
      animatedFiles.push_back(std::vector<clFile>());
      removeArgs(ac,av,i,1); --i;
    } else if (arg == "--file") {
      inAnimation = false;
      removeArgs(ac,av,i,1); --i;
    } else if (arg == "-w" || arg == "--width") {
      width = atoi(av[i+1]);
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "-h" || arg == "--height") {
      height = atoi(av[i+1]);
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "-vp" || arg == "--eye") {
      pos.x = atof(av[i+1]);
      pos.y = atof(av[i+2]);
      pos.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "-vu" || arg == "--up") {
      up.x = atof(av[i+1]);
      up.y = atof(av[i+2]);
      up.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "-vi" || arg == "--gaze") {
      gaze.x = atof(av[i+1]);
      gaze.y = atof(av[i+2]);
      gaze.z = atof(av[i+3]);
      removeArgs(ac,av,i,4); --i;
    } else if (arg == "-fv" || arg == "--fovy") {
      fovy = atof(av[i+1]);
      removeArgs(ac,av,i,2); --i;
    } else if (arg == "-ar") {
      apertureRadius = atof(av[i+1]);
      removeArgs(ac,av,i,2); --i;
    } else if (arg.compare(0, 4, "-sg:") == 0) {
	// SG parameters are validated by prefix only.
	// Later different function is used for parsing this type parameters.
	continue;
    } else if (arg[0] != '-') {
      if (!inAnimation)
        files.push_back(clFile(av[i], currentCLTransform));
      else
        animatedFiles.back().push_back(clFile(av[i], currentCLTransform));
      currentCLTransform = clTransform();
      removeArgs(ac,av,i,1); --i;
    } else {
        std::cerr << "Error: unknown parameter '" << arg << "'." <<  std::endl;
	return 1;
    }
  }
  return 0;
}

void OSPApp::parseCommandLineSG(int ac, const char **&av, sg::Node &root) {
  for (int i = 1; i < ac; i++) {
    std::string arg(av[i]);
	printf("cmd: %s\n", arg.c_str());
    size_t f;
    std::string value("");

    // Only parameters started with "-sg:" are allowed
    if (arg.compare(0, 4, "-sg:") != 0)
      continue;

    // Store original argument before processing
    const std::string orgarg(arg);

    // Remove "-sg:" prefix
    arg = arg.substr(4, arg.length());

    while ((f = arg.find(":")) != std::string::npos ||
           (f = arg.find(",")) != std::string::npos) {
      arg[f] = ' ';
    }

    f = arg.find("+=");
    bool addNode = false;
    if (f != std::string::npos) {
      value = arg.substr(f + 2, arg.size());
      addNode = true;
    } else {
      f = arg.find("=");
      if (f != std::string::npos)
        value = arg.substr(f + 1, arg.size());
    }
    if (value != "") {
      std::stringstream ss;
      ss << arg.substr(0, f);
      std::string child;
      std::reference_wrapper<sg::Node> node_ref = root;
      try {
        while (ss >> child) {
          node_ref = node_ref.get().childRecursive(child);
        }
      }
      catch (const std::runtime_error &) {
        std::cerr << "Warning: unknown sg::Node '" << child
                  << "', ignoring option '" << orgarg << "'." << std::endl;
      }
      auto &node = node_ref.get();

      std::stringstream vals(value);

      if (addNode) {
        std::string name, type;
        vals >> name >> type;
        try {
          node.createChild(name, type);
        }
        catch (const std::runtime_error &) {
          std::cerr << "Warning: unknown sg::Node type '" << type
                    << "', ignoring option '" << orgarg << "'." << std::endl;
        }
      } else { // set node value

        // TODO: more generic implementation
        if (node.valueIsType<std::string>()) {
          node.setValue(value);
        } else if (node.valueIsType<float>()) {
          float x;
          vals >> x;
          node.setValue(x);
        } else if (node.valueIsType<int>()) {
          int x;
          vals >> x;
          node.setValue(x);
        } else if (node.valueIsType<bool>()) {
          bool x;
          vals >> x;
          node.setValue(x);
        } else if (node.valueIsType<ospcommon::vec3f>()) {
          float x, y, z;
          vals >> x >> y >> z;
          node.setValue(ospcommon::vec3f(x, y, z));
        } else if (node.valueIsType<ospcommon::vec2i>()) {
          int x, y;
          vals >> x >> y;
          node.setValue(ospcommon::vec2i(x, y));
        } else
          try {
            auto &vec = dynamic_cast<sg::DataVector1f &>(node);
            float f;
            while (vals.good()) {
              vals >> f;
              vec.push_back(f);
            }
          }
        catch (...) {
	  std::cerr << "Unexpected exception" << std::endl;
        }
      }
    }
  }
}

void OSPApp::addLightsToScene(sg::Node &renderer, bool defaults) {
  auto &lights = renderer["lights"];

  if (defaults) {
    auto &sun = lights.createChild("sun", "DirectionalLight");
    sun["color"] = vec3f(1.f, 232.f / 255.f, 166.f / 255.f);
    sun["direction"] = vec3f(0.462f, -1.f, -.1f);
    sun["intensity"] = 1.5f;

    auto &bounce = lights.createChild("bounce", "DirectionalLight");
    bounce["color"] = vec3f(127.f / 255.f, 178.f / 255.f, 255.f / 255.f);
    bounce["direction"] = vec3f(-.93, -.54f, -.605f);
    bounce["intensity"] = 0.25f;

    if (hdri_light == "") {
      auto &ambient = lights.createChild("ambient", "AmbientLight");
      ambient["intensity"] = 0.9f;
      ambient["color"] = vec3f(174.f / 255.f, 218.f / 255.f, 255.f / 255.f);
    }
  }

  if (hdri_light != "") {
    auto tex = sg::Texture2D::load(hdri_light, false);
    tex->setName("map");
    auto &hdri = lights.createChild("hdri", "HDRILight");
    tex->traverse("verify");
    tex->traverse("commit");
    hdri.add(tex);
  }
}

void OSPApp::addImporterNodesToWorld(sg::Node &renderer) {
  auto &world = renderer["world"];
  auto &animation = renderer["animationcontroller"];

  for (auto file : files) {
    FileName fn = file.file;
    if (fn.ext() == "ospsg")
      sg::loadOSPSG(renderer.shared_from_this(), fn.str());
    else {
      // create material array
      for (int i = 0; i < matrix_i; i++) {
        for (int j = 0; j < matrix_j; j++) {
          for (int k = 0; k < matrix_k; k++) {
            std::stringstream ss;
            ss << fn.name() << "_" << i << "_" << j << "_" << k;
            auto importerNode_ptr =
                sg::createNode(ss.str(), "Importer")->nodeAs<sg::Importer>();
            ;
            auto &importerNode = *importerNode_ptr;

            auto &transform =
                world.createChild("transform_" + ss.str(), "Transform");
            transform.add(importerNode_ptr);
            importerNode["fileName"] = fn.str();

            transform["scale"] = file.transform.scale;
            transform["rotation"] = file.transform.rotation;
            if (files.size() < 2 && animatedFiles.empty()) {
              auto &rotation =
                  transform["rotation"].createChild("animator", "Animator");

              rotation.traverse("verify");
              rotation.traverse("commit");
              rotation.child("value1") = vec3f(0.f, 0.f, 0.f);
              rotation.child("value2") = vec3f(0.f, 2.f * 3.14f, 0.f);

              animation.setChild("rotation", rotation.shared_from_this());
            }

            renderer.traverse("verify");
            renderer.traverse("commit");
            auto bounds = importerNode_ptr->computeBounds();
            auto size = bounds.upper - bounds.lower;
            float maxSize = max(max(size.x, size.y), size.z);
            if (!std::isfinite(maxSize))
              maxSize = 0.f; // FIXME: why is maxSize = NaN in some cases?!
            vec3f offset = { i * maxSize * 1.3f, j * maxSize * 1.3f,
                             k * maxSize * 1.3f };
            transform["position"] = file.transform.translate + offset;
          }
        }
      }
    }
  }
}

void OSPApp::addAnimatedImporterNodesToWorld(sg::Node &renderer) {
  auto &world = renderer["world"];
  auto &animation = renderer["animationcontroller"];

  for (auto &animatedFile : animatedFiles) {
    if (animatedFile.empty())
      continue;

    auto &transform =
        world.createChild("transform_" + animatedFile[0].file, "Transform");

    transform["scale"] = animatedFile[0].transform.scale;
    transform["position"] = animatedFile[0].transform.translate;
    transform["rotation"] = animatedFile[0].transform.rotation;
    auto &selector =
        transform.createChild("selector_" + animatedFile[0].file, "Selector");

    for (auto file : animatedFile) {
      FileName fn = file.file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer.shared_from_this(), fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"] = fn.str();
        selector.add(importerNode_ptr);
      }
    }
    auto &anim_selector = selector["index"].createChild(
        "anim_" + animatedFile[0].file, "Animator");

    anim_selector.traverse("verify");
    anim_selector.traverse("commit");
    anim_selector["value2"] = int(animatedFile.size());
    animation.setChild("anim_selector", anim_selector.shared_from_this());
  }
}

} // ::app
} // ::ospray
