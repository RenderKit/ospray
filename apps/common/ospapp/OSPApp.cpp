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

#include "ospcommon/utility/StringManip.h"

#include "OSPApp.h"
#include "common/sg/SceneGraph.h"
#include "sg/geometry/TriangleMesh.h"
#include "sg/visitor/PrintNodes.h"
#include "sg/visitor/VerifyNodes.h"
#include "sg/module/Module.h"

namespace ospray {
  namespace app {

    int OSPApp::initializeOSPRay(int *argc, const char *argv[])
    {
      int init_error = ospInit(argc, argv);
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

    void OSPApp::printHelp()
    {
      std::cout << "./ospApp [params] -sg:[params] [files]" << std::endl;
      std::cout << "params..." << std::endl
                << "\t" << "-f --fast //prioritizes performance over advanced rendering features" << std::endl
                << "\t" << "-sg:node:...:node=value //modify a node value" << std::endl
                << "\t" << "-sg:node:...:node+=name,type //modify a node value" << std::endl
                << "\t" << "-r --renderer //renderer type.  scivis, pathtracer, ao1, raycast" << std::endl
                << "\t" << "-d --debug //debug output" << std::endl
                << "\t" << "-m --module //load custom ospray module" << std::endl
                << "\t" << "--matrix int int int //create an array of load models of dimensions xyz" << std::endl
                << "\t" << "--add-plane //add a ground plane (default for non-fast mode)" << std::endl
                << "\t" << "--no-plane //remove ground plane" << std::endl
                << "\t" << "--no-lights //no default lights" << std::endl
                << "\t" << "--add-lights //default lights" << std::endl
                << "\t" << "--hdri-light filename //add an hdri light" << std::endl
                << "\t" << "--translate float float float //translate transform" << std::endl
                << "\t" << "--scale float float float //scale transform" << std::endl
                << "\t" << "--rotate float float float //rotate transform" << std::endl
                << "\t" << "--animation //adds subsequent import files to a timeseries" << std::endl
                << "\t" << "--file //adds subsequent import files without a timeseries" << std::endl
                << "\t" << "-w int //window width" << std::endl
                << "\t" << "-h int //window height" << std::endl
                << "\t" << "--size int int //window width height" << std::endl
                << "\t" << "-vp float float float //camera position xyz" << std::endl
                << "\t" << "-vu float float float //camera up xyz" << std::endl
                << "\t" << "-vi float float float //camera direction xyz" << std::endl
                << "\t" << "-vf float //camera field of view" << std::endl
                << "\t" << "-ar float //camera aperture radius" << std::endl
                << std::endl;
    }

    int OSPApp::main(int argc, const char *argv[])
    {
      int result = initializeOSPRay(&argc, argv);
      if (result != 0)
        return result;

      // access/load symbols/sg::Nodes dynamically
      loadLibrary("ospray_sg");

      parseGeneralCommandLine(argc, argv);

      auto rendererPtr = sg::createNode("renderer", "Renderer");
      auto &renderer = *rendererPtr;

      if (!initialRendererType.empty())
        renderer["rendererType"] = initialRendererType;

      renderer.createChild("animationcontroller", "AnimationController");

      if (fast) {
        renderer["spp"] = -1;
        renderer["shadowsEnabled"] = false;
        renderer["aoTransparencyEnabled"] = false;
        renderer["minContribution"] = 0.1f;
        renderer["maxDepth"] = 3;
        renderer["frameBuffer"]["toneMapping"] = false;
        renderer["frameBuffer"]["useVarianceBuffer"] = false;
        addPlane = false;
      }

      addLightsToScene(renderer);
      addImporterNodesToWorld(renderer);
      addAnimatedImporterNodesToWorld(renderer);
      addPlaneToScene(renderer);
      setupCamera(renderer);

      renderer["frameBuffer"]["size"] = vec2i(width, height);
      renderer.traverse(sg::VerifyNodes{});
      renderer.commit();

      // last, to be able to modify all created SG nodes
      parseCommandLineSG(argc, argv, renderer);

      if (debug)
        renderer.traverse(sg::PrintNodes{});

      // recommit in case any command line options modified the scene graph
      renderer.traverse(sg::VerifyNodes{});
      renderer.commit();

      render(rendererPtr);

      return 0;
    }

    void OSPApp::parseGeneralCommandLine(int &ac, const char **&av)
    {
      // Call children command line parsing methods
      if (parseCommandLine(ac, av) != 0)
        return;

      clTransform currentCLTransform;
      bool inAnimation = false;
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "--help") {
          printHelp();
          // We don't want to do anything else so return 2.
          std::exit(0);
        } else if (arg == "-r" || arg == "--renderer") {
          initialRendererType = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-d" || arg == "--debug") {
          debug = true;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "-m" || arg == "--module") {
          ospLoadModule(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-m" || arg == "--sg:module") {
          sg::loadModule(av[i+1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "--matrix") {
          matrix_i = atoi(av[i + 1]);
          matrix_j = atoi(av[i + 2]);
          matrix_k = atoi(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
        } else if (arg == "--add-plane") {
          addPlane = true;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--no-plane") {
          addPlane = false;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--no-lights") {
          noDefaultLights = true;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--add-lights") {
          addDefaultLights = true;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--hdri-light") {
          hdriLightFile = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "--translate") {
          currentCLTransform.translate.x = atof(av[i + 1]);
          currentCLTransform.translate.y = atof(av[i + 2]);
          currentCLTransform.translate.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
        } else if (arg == "--scale") {
          currentCLTransform.scale.x = atof(av[i + 1]);
          currentCLTransform.scale.y = atof(av[i + 2]);
          currentCLTransform.scale.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
        } else if (arg == "--rotate") {
          currentCLTransform.rotation.x = atof(av[i + 1]);
          currentCLTransform.rotation.y = atof(av[i + 2]);
          currentCLTransform.rotation.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
        } else if (arg == "--animation") {
          inAnimation = true;
          animatedFiles.push_back(std::vector<clFile>());
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--fast" || arg == "-f") {
          fast = true;
        } else if (arg == "--no-fast" || arg == "-nf") {
          fast = false;
        } else if (arg == "--file") {
          inAnimation = false;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "-w" || arg == "--width") {
          width = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-h" || arg == "--height") {
          height = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "--size") {
          width = atoi(av[i + 1]);
          height = atoi(av[i + 2]);
          removeArgs(ac, av, i, 3);
          --i;
        } else if (arg == "-vp") {
          vec3f posVec;
          posVec.x = atof(av[i + 1]);
          posVec.y = atof(av[i + 2]);
          posVec.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
          pos = posVec;
        } else if (arg == "-vu") {
          vec3f upVec;
          upVec.x = atof(av[i + 1]);
          upVec.y = atof(av[i + 2]);
          upVec.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
          up = upVec;
        } else if (arg == "-vi") {
          vec3f gazeVec;
          gazeVec.x = atof(av[i + 1]);
          gazeVec.y = atof(av[i + 2]);
          gazeVec.z = atof(av[i + 3]);
          removeArgs(ac, av, i, 4);
          --i;
          gaze = gazeVec;
        } else if (arg == "-fv") {
          fovy = atof(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-ar") {
          apertureRadius = atof(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg.compare(0, 4, "-sg:") == 0) {
          // SG parameters are validated by prefix only.
          // Later different function is used for parsing this type parameters.
          continue;
        } else if (arg[0] != '-' || utility::beginsWith(arg, "--import:")) {
          if (!inAnimation)
            files.push_back(clFile(av[i], currentCLTransform));
          else
            animatedFiles.back().push_back(clFile(av[i], currentCLTransform));
          currentCLTransform = clTransform();
          removeArgs(ac, av, i, 1);
          --i;
        } else {
          std::cerr << "Error: unknown parameter '" << arg << "'." << std::endl;
          printHelp();
          std::exit(1);
        }
      }
    }

    void OSPApp::parseCommandLineSG(int ac, const char **&av, sg::Node &root)
    {
      for (int i = 1; i < ac; i++) {
        std::string arg(av[i]);
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
                        << "', ignoring option '" << orgarg << "'."
                        << std::endl;
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

    void OSPApp::addLightsToScene(sg::Node &renderer)
    {
      renderer.verify();
      renderer.commit();
      auto &lights = renderer["lights"];

      if (noDefaultLights == false &&
          (lights.numChildren() <= 0 || addDefaultLights == true)) {
        if (!fast) {
          auto &sun = lights.createChild("sun", "DirectionalLight");
          sun["color"] = vec3f(1.f, 247.f / 255.f, 201.f / 255.f);
          sun["direction"] = vec3f(0.462f, -1.f, -.1f);
          sun["intensity"] = 3.0f;
          sun["angularDiameter"] = 0.8f;

          auto &bounce = lights.createChild("bounce", "DirectionalLight");
          bounce["color"] = vec3f(202.f / 255.f, 216.f / 255.f, 255.f / 255.f);
          bounce["direction"] = vec3f(-.93, -.54f, -.605f);
          bounce["intensity"] = 1.25f;
          bounce["angularDiameter"] = 3.0f;
        }

        if (hdriLightFile == "") {
          auto &ambient = lights.createChild("ambient", "AmbientLight");
          ambient["intensity"] = fast ? 1.25f : 0.9f;
          ambient["color"] = fast ?
              vec3f(1.f) : vec3f(217.f / 255.f, 230.f / 255.f, 255.f / 255.f);
        }
      }

      if (hdriLightFile != "") {
        auto tex = sg::Texture2D::load(hdriLightFile, false);
        tex->setName("map");
        auto &hdri = lights.createChild("hdri", "HDRILight");
        tex->verify();
        tex->commit();
        hdri.add(tex);
      }
      renderer.verify();
      renderer.commit();
    }

    void OSPApp::addImporterNodesToWorld(sg::Node &renderer)
    {
      auto &world = renderer["world"];
      auto &animation = renderer["animationcontroller"];

      for (auto file : files) {
        FileName fn = file.file;
        if (fn.ext() == "ospsg")
        {
          auto& cam = renderer["camera"];
          auto dirTS = cam["dir"].lastModified();
          auto posTS = cam["pos"].lastModified();
          auto upTS = cam["up"].lastModified();
          sg::loadOSPSG(renderer.shared_from_this(), fn.str());
          if (cam["dir"].lastModified() > dirTS)
            gaze = (cam["pos"].valueAs<vec3f>() + cam["dir"].valueAs<vec3f>());
          if (cam["pos"].lastModified() > posTS)
            pos = cam["pos"].valueAs<vec3f>();
          if (cam["up"].lastModified() > upTS)
            up = cam["up"].valueAs<vec3f>();
        }
        else {
          // create material array
          for (int i = 0; i < matrix_i; i++) {
            for (int j = 0; j < matrix_j; j++) {
              for (int k = 0; k < matrix_k; k++) {
                std::stringstream ss;
                ss << fn.name() << "_" << i << "_" << j << "_" << k;
                auto importerNode_ptr =
                    sg::createNode(ss.str(),"Importer")->nodeAs<sg::Importer>();
                auto &importerNode = *importerNode_ptr;

                auto &transform =
                    world.createChild("transform_" + ss.str(), "Transform");
                transform.add(importerNode_ptr);
                importerNode["fileName"] = fn.str();

                if (fast) {
                  if (importerNode.hasChildRecursive("gradientShadingEnabled"))
                    importerNode.childRecursive("gradientShadingEnabled") = false;
                  if (importerNode.hasChildRecursive("adaptiveMaxSamplingRate"))
                    importerNode.childRecursive("adaptiveMaxSamplingRate") = 0.2f;
                }

                transform["scale"] = file.transform.scale;
                transform["rotation"] = file.transform.rotation;
                if (files.size() < 2 && animatedFiles.empty()) {
                  auto &rotation =
                      transform["rotation"].createChild("animator", "Animator");

                  rotation.verify();
                  rotation.commit();
                  rotation.child("value1") = vec3f(0.f, 0.f, 0.f);
                  rotation.child("value2") = vec3f(0.f, 2.f * 3.14f, 0.f);

                  animation.setChild("rotation", rotation.shared_from_this());
                }

                renderer.verify();
                renderer.commit();
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

    void OSPApp::setupCamera(sg::Node &renderer)
    {
      auto &world = renderer["world"];
      auto bbox = bboxWithoutPlane;
      if (bbox.empty())
        bbox = world.bounds();
      vec3f diag = bbox.size();
      diag = max(diag, vec3f(0.3f * length(diag)));
      if (!gaze.isOverridden())
        gaze = ospcommon::center(bbox);

      if (!pos.isOverridden())
        pos = gaze.getValue() -
              .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
      if (!up.isOverridden())
        up = vec3f(0.f, 1.f, 0.f);

      auto &camera = renderer["camera"];
      camera["pos"] = pos.getValue();
      camera["dir"] = normalize(gaze.getValue() - pos.getValue());
      camera["up"] = up.getValue();

      // NOTE: Stash computed gaze point in the camera for apps, invalid once
      //       camera moves unless also updated by the app!
      camera.createChild("gaze", "vec3f", gaze.getValue());

      if (camera.hasChild("fovy"))
        camera["fovy"] = fovy.getValue();
      if (camera.hasChild("apertureRadius"))
        camera["apertureRadius"] = apertureRadius.getValue();
      if (camera.hasChild("focusdistance"))
        camera["focusdistance"] = length(pos.getValue() - gaze.getValue());
      renderer.verify();
      renderer.commit();
    }

    void OSPApp::addAnimatedImporterNodesToWorld(sg::Node &renderer)
    {
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
            transform.createChild("selector_" + animatedFile[0].file,
                                  "Selector");

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

        anim_selector.verify();
        anim_selector.commit();
        anim_selector["value2"] = int(animatedFile.size());
        animation.setChild("anim_selector", anim_selector.shared_from_this());
      }
    }

    void OSPApp::addPlaneToScene(sg::Node &renderer)
    {
      auto &world = renderer["world"];
      if (addPlane == false) {
        return;
      }
      auto bbox = world.bounds();
      if (bbox.empty()) {
        bbox.lower = vec3f(-5, 0, -5);
        bbox.upper = vec3f(5, 10, 5);
      }
      bboxWithoutPlane = bbox;

      float ps = bbox.upper.x * 3.f;
      float py = bbox.lower.y + 0.01f;

      auto position = std::make_shared<sg::DataVector3f>();
      position->push_back(vec3f{ -ps, py, -ps });
      position->push_back(vec3f{ -ps, py, ps });
      position->push_back(vec3f{ ps, py, -ps });
      position->push_back(vec3f{ ps, py, ps });
      position->setName("vertex");

      auto index = std::make_shared<sg::DataVector3i>();
      index->push_back(vec3i{ 0, 1, 2 });
      index->push_back(vec3i{ 1, 2, 3 });
      index->setName("index");

      auto &plane = world.createChild("plane", "TriangleMesh");

      auto sg_plane = plane.nodeAs<sg::TriangleMesh>();
      sg_plane->add(position);
      sg_plane->add(index);
      auto &planeMaterial =
          (*plane["materialList"].nodeAs<sg::MaterialList>())[0];
      planeMaterial["Kd"] = vec3f(0.3f);
      planeMaterial["Ks"] = vec3f(0.0f);
      planeMaterial["Ns"] = 10.f;

      renderer.verify();
      renderer.commit();
    }

  } // ::ospray::app
} // ::ospray
