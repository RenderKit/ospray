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
#include "sg/generator/Generator.h"
#include "sg/module/Module.h"
#include "sg/texture/Texture2D.h"
#include "sg/visitor/PrintNodes.h"
#include "sg/visitor/VerifyNodes.h"

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
      std::cout <<
R"text(
./ospApp [parameters] [scene_files]

general app-parameters:

    -f --fast
        prioritizes performance over advanced rendering features
    -sg:node:...:node=value
        modify a node value
    -sg:node:...:node+=name,type
        modify a node value
    -r --renderer [renderer_type]
        renderer type --> scivis, pathtracer, ao, raycast, etc...
    -d --debug
        debug output
    -m --module [module_name]
        load custom ospray module
    --matrix [int] [int] [int]
        create an array of load models of dimensions xyz
    --add-plane
        add a ground plane
    --no-plane
        remove ground plane
    --no-lights
        no default lights
    --add-lights
        default lights
    -tf --transferFunction [string]
        default transferFunction
    -ltf --loadTransferFunction [file]
        load transfer function preset from file
    --hdri-light [filename]
        add an hdri light
    --translate [float] [float] [float]
        translate transform
    --scale [float] [float] [float]
        scale transform
    --rotate [float] [float] [float]
        rotate transform
    --animation
        adds subsequent import files to a timeseries
    --static
        adds subsequent import files without a timeseries
    -w [int]
        window width
    -h [int]
        window height
    --size [int] [int]
        window width height
    -sd
        alias for window size = 640x480
    -hd
        alias for window size = 1920x1080
    -4k
        alias for window size = 3840x2160
    -8k
        alias for window size = 7680x4320
    -vp [float] [float] [float]
        camera position xyz
    -vu [float] [float] [float]
        camera up xyz
    -vi [float] [float] [float]
        camera direction xyz
    -vf [float]
        camera field of view
    -ar [float]
        camera aperture radius
    --aces
        use ACES tone mapping
    --filmic
        use filmic tone mapping

scene data generators through command line options:

usage --> "--generate:type[:parameter1=value,parameter2=value,...]"

    types:

      basicVolume: generate a volume with linearly increasing voxel values
          parameters:
              [dimensions,dims]=[intxintxint]
                  the 3D dimensions of the volume

      cylinders: generate a block of cylinders in {X,Y} grid of length Z
          parameters:
              [dimensions,dims]=[intxintxint]
                  number of cylinders to generate in X,Y 2D dimensions, use Z for length
              radius=[float]
                  radius of cylinders

      cube: generate a simple cube as a QuadMesh

      gridOfSpheres: generate a block of gridded sphere centers of uniform radius
          parameters:
              [dimensions,dims]=[intxintxint]
                  number of spheres to generate in each 3D dimension

      spheres: generate a block of random sphere centers of uniform radius
          parameters:
              numSpheres=[int]
                  number of spheres to generate
              radius=[float]
                  radius of spheres

      unstructuredHex: generate a simple unstructured volume as hexes

      unstructuredTet: generate a simple unstructured volume as tets

      unstructuredWedge: generate a simple unstructured volume as wedges

      vtkWavelet: generate the vtkWavelet volume (requries VTK support compiled in)
          parameters:
              [dimensions,dims]=[intxintxint]
                  number of spheres to generate in each 3D dimension
              [isovalues,isosurfaces]=[value1/value2/value3...]
                  use vtkMarchingCubes filter to generate isosurfaces instead of the volume
              viewSlice
                  add a slice to the middle of the volume in the X/Y plane
)text"
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

      auto rootPtr = sg::createNode("frame0", "Frame")->nodeAs<sg::Frame>();

      auto &root     = *rootPtr;
      auto &renderer = root["renderer"];

      if (!initialRendererType.empty())
        renderer["rendererType"] = initialRendererType;

      renderer.createChild("animationcontroller", "AnimationController");
      renderer.createChild("transferFunctions", "Node");

      //transfer function presets
      auto& tfPresets = renderer.createChild("transferFunctionPresets", "Node");

      auto addPreset = [&](std::string name,
                           std::vector<vec3f> colors,
                           std::shared_ptr<sg::Node>presets) {
        auto& preset = *presets->createChild(name, "TransferFunction").nodeAs<sg::TransferFunction>();
        auto& colors4f = *preset["colorControlPoints"].nodeAs<sg::DataVector4f>();
        colors4f.clear();
        for(size_t i = 0; i < colors.size(); i++) {
          colors4f.push_back(vec4f(i/float(colors.size()-1), colors[i].x,
              colors[i].y, colors[i].z));
        }
        preset.updateChildDataValues();
      };

      std::vector<vec3f> colors;
      // The presets have no existing opacity value
      const std::vector<vec2f> opacities;
      // From the old volume viewer, these are based on ParaView
      // Jet transfer function
      colors.push_back(vec3f(0       , 0, 0.562493));
      colors.push_back(vec3f(0       , 0, 1       ));
      colors.push_back(vec3f(0       , 1, 1       ));
      colors.push_back(vec3f(0.500008, 1, 0.500008));
      colors.push_back(vec3f(1       , 1, 0       ));
      colors.push_back(vec3f(1       , 0, 0       ));
      colors.push_back(vec3f(0.500008, 0, 0       ));
      addPreset("Jet", colors, tfPresets.shared_from_this());
      colors.clear();

      colors.push_back(vec3f(0        , 0          , 0          ));
      colors.push_back(vec3f(0        , 0.120394   , 0.302678   ));
      colors.push_back(vec3f(0        , 0.216587   , 0.524575   ));
      colors.push_back(vec3f(0.0552529, 0.345022   , 0.659495   ));
      colors.push_back(vec3f(0.128054 , 0.492592   , 0.720287   ));
      colors.push_back(vec3f(0.188952 , 0.641306   , 0.792096   ));
      colors.push_back(vec3f(0.327672 , 0.784939   , 0.873426   ));
      colors.push_back(vec3f(0.60824  , 0.892164   , 0.935546   ));
      colors.push_back(vec3f(0.881376 , 0.912184   , 0.818097   ));
      colors.push_back(vec3f(0.9514   , 0.835615   , 0.449271   ));
      colors.push_back(vec3f(0.904479 , 0.690486   , 0          ));
      colors.push_back(vec3f(0.854063 , 0.510857   , 0          ));
      colors.push_back(vec3f(0.777096 , 0.330175   , 0.000885023));
      colors.push_back(vec3f(0.672862 , 0.139086   , 0.00270085 ));
      colors.push_back(vec3f(0.508812 , 0          , 0          ));
      colors.push_back(vec3f(0.299413 , 0.000366217, 0.000549325));
      colors.push_back(vec3f(0.0157473, 0.00332647 , 0          ));
      addPreset("Ice Fire", colors, tfPresets.shared_from_this());
      colors.clear();

      colors.push_back(vec3f(0.231373, 0.298039 , 0.752941));
      colors.push_back(vec3f(0.865003, 0.865003 , 0.865003));
      colors.push_back(vec3f(0.705882, 0.0156863, 0.14902));
      addPreset("Cool Warm", colors, tfPresets.shared_from_this());
      colors.clear();

      colors.push_back(vec3f(0, 0, 1));
      colors.push_back(vec3f(1, 0, 0));
      addPreset("Blue Red", colors, tfPresets.shared_from_this());
      colors.clear();

      colors.push_back(vec3f(0));
      colors.push_back(vec3f(1));
      addPreset("Grayscale", colors, tfPresets.shared_from_this());
      colors.clear();

      for (const auto& tfFile : tfFiles) {
        auto& tf = *renderer["transferFunctionPresets"].createChild("loadedTF",
          "TransferFunction").nodeAs<sg::TransferFunction>();
        tf.loadParaViewTF(tfFile);
      }

      auto &framebuffer = root["frameBuffer"];

      if (fast) {
        renderer["spp"] = -1;
        renderer["shadowsEnabled"] = false;
        renderer["aoTransparencyEnabled"] = false;
        renderer["minContribution"] = 0.1f;
        renderer["maxDepth"] = 3;

        framebuffer["useVarianceBuffer"] = false;
        addPlane = false;
      }

      addLightsToScene(renderer);
      addImporterNodesToWorld(root);
      addGeneratorNodesToWorld(renderer);
      addAnimatedImporterNodesToWorld(renderer);

      framebuffer["size"] = vec2i(width, height);
      auto &navFB = root.createChild("navFrameBuffer", "FrameBuffer");
      navFB["useAccumBuffer"] = false;
      navFB["useVarianceBuffer"] = false;

      setupToneMapping(framebuffer, navFB);

      root.traverse(sg::VerifyNodes(true));
      root.commit();

      // sensible default for orthographic camera, before command line parsing
      auto &camera = root["camera"];
      if (camera.hasChild("height")) {
        auto bbox = renderer["world"].bounds();
        camera["height"] = bbox.empty() ? 1.f : 0.5f * length(bbox.size());
      }

      // last, to be able to modify all created SG nodes
      parseCommandLineSG(argc, argv, root);

      // after parseCommandLineSG (may have changed world bounding box)
      addPlaneToScene(renderer);
      setupCamera(root);

      if (debug)
        root.traverse(sg::PrintNodes{});

      render(rootPtr);

      rootPtr.reset();
      ospShutdown();

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
        } else if (arg == "--transferFunction" ||
          arg == "-tf") {
          defaultTransferFunction = std::string(av[i+1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "--loadTransferFunction" ||
          arg == "-ltf") {
          tfFiles.push_back(av[i+1]);
          removeArgs(ac, av, i, 2);
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
        } else if (arg == "--static" || arg == "--file") {
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
        } else if (arg == "-sd") {
          width  = 640;
          height = 480;
        } else if (arg == "-hd") {
          width  = 1920;
          height = 1080;
        } else if (arg == "-4k") {
          width  = 3840;
          height = 2160;
        } else if (arg == "-8k") {
          width  = 7680;
          height = 4320;
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
        } else if (arg == "--aces") {
          aces = true;
          filmic = false;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--filmic") {
          filmic = true;
          aces = false;
          removeArgs(ac, av, i, 1);
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
        } else if (utility::beginsWith(arg, "--generate:")) {
          auto splitValues = utility::split(arg, ':');
          auto type = splitValues[1];
          std::string params = splitValues.size() > 2 ? splitValues[2] : "";
          generators.push_back({type, params});
          removeArgs(ac, av, i, 1);
          --i;
        } else {
          std::cerr << "Error: unknown parameter '" << arg << "'." << std::endl;
          printHelp();
          std::exit(1);
        }
      }
    }

    void OSPApp::parseCommandLineSG(int ac, const char **&av, sg::Frame &root)
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
          std::vector<std::shared_ptr<sg::Node>> children;
          while (ss >> child) {
            try {
              if (ss.eof())
                children = node_ref.get().childrenRecursive(child);
              else
                node_ref = node_ref.get().childRecursive(child);
            } catch (...) {
              std::cerr << "Warning: could not find child: " << child << std::endl;
            }
          }

          if (children.empty()) {
            std::cerr << "Warning: no children found for " << av[i] << " lookup\n";
            continue;
          }

          std::stringstream vals(value);

          if (addNode) {
            auto &node = *children[0];
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
            for (auto nodePtr : children)
            {
              auto &node = *nodePtr;
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
              } else if (node.valueIsType<ospcommon::vec3i>()) {
                int x, y, z;
                vals >> x >> y >> z;
                node.setValue(ospcommon::vec3i(x, y, z));
              } else if (node.valueIsType<ospcommon::vec2i>()) {
                int x, y;
                vals >> x >> y;
                node.setValue(ospcommon::vec2i(x, y));
              } else if (node.valueIsType<ospcommon::vec2f>()) {
                float x, y;
                vals >> x >> y;
                node.setValue(ospcommon::vec2f(x, y));
              } else {
                try {
                  auto &vec = dynamic_cast<sg::DataVector1f &>(node);
                  float f;
                  while (vals.good()) {
                    vals >> f;
                    vec.push_back(f);
                  }
                } catch (...) {
                  std::cerr << "Cannot set value of node '" << node.name()
                            << "' on the command line!"
                            << " The expected value type is not (yet) handled."
                            << std::endl;
                }
              }
            }
          }
        }
      }
    }

    void OSPApp::addLightsToScene(sg::Node &renderer)
    {
      auto &lights = renderer["lights"];

      if (noDefaultLights == false &&
          (lights.numChildren() <= 0 || addDefaultLights == true)) {
        if (!fast) {
          auto &sun = lights.createChild("sun", "DirectionalLight");
          sun["color"] = vec3f(1.f, 247.f / 255.f, 201.f / 255.f);
          sun["direction"] = vec3f(0.462f, -1.f, -.1f);
          sun["intensity"] = 3.0f;
          sun["angularDiameter"] = 0.53f;

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
        hdri.add(tex);
        renderer.verify(); //TODO: this should not be necessary
        sg::Texture2D::clearTextureCache();
      }
    }

    void OSPApp::addImporterNodesToWorld(sg::Node &root)
    {
      auto &renderer = root["renderer"];
      auto &world = renderer["world"];
      auto &animation = renderer["animationcontroller"];

      for (auto file : files) {
        FileName fn = file.file;
        if (fn.ext() == "ospsg")
        {
          auto& cam = root["camera"];
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

                auto transferFunctions = importerNode.childrenRecursive("transferFunction");
                for (auto tf : transferFunctions)
                  renderer["transferFunctions"].add(tf);

                transform["scale"] = file.transform.scale;
                transform["rotation"] = file.transform.rotation;
                if (files.size() < 2 && animatedFiles.empty()) {
                  auto &rotation =
                      transform["rotation"].createChild("animator", "Animator");

                  rotation.createChild("value1", "vec3f", vec3f(0.f, 0.f, 0.f));
                  rotation.createChild("value2", "vec3f", vec3f(0.f, 2.f * 3.14f, 0.f));
                  rotation.setValue(vec3f(0.0f, 0.0f, 0.0f));

                  animation.setChild("rotation", rotation.shared_from_this());
                }

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

    void OSPApp::addGeneratorNodesToWorld(sg::Node &renderer)
    {
      auto &world = renderer["world"];

      for (const auto &g : generators) {
        auto generatorNode =
          world.createChild("generator", "Generator").nodeAs<sg::Generator>();

        generatorNode->child("generatorType") = g.type;
        generatorNode->child("parameters")    = g.params;

        if (fast) {
          if (generatorNode->hasChildRecursive("gradientShadingEnabled"))
            generatorNode->childRecursive("gradientShadingEnabled") = false;
          if (generatorNode->hasChildRecursive("adaptiveMaxSamplingRate"))
            generatorNode->childRecursive("adaptiveMaxSamplingRate") = 0.2f;
        }

        generatorNode->generateData();
      }
    }

    void OSPApp::setupCamera(sg::Node &root)
    {
      auto &renderer = root["renderer"];
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

      auto &camera = root["camera"];
      camera["pos"] = pos.getValue();
      camera["dir"] = normalize(gaze.getValue() - pos.getValue());
      camera["up"] = up.getValue();

      // XXX hack: consumed and removed in constructor of ImGuiViewer
      camera.createChild("gaze", "vec3f", gaze.getValue());

      if (camera.hasChild("fovy") && fovy.isOverridden())
        camera["fovy"] = fovy.getValue();
      if (camera.hasChild("apertureRadius") && apertureRadius.isOverridden())
        camera["apertureRadius"] = apertureRadius.getValue();
      if (camera.hasChild("focusdistance"))
        camera["focusdistance"] = length(pos.getValue() - gaze.getValue());
      if (camera.hasChild("aspect"))
        camera["aspect"] = width / (float)height;
    }

    void OSPApp::setupToneMapping(sg::Node &frameBuffer, sg::Node &fb2)
    {
      auto &toneMapper = frameBuffer.createChild("toneMapper", "ToneMapper");
      toneMapper["enabled"] = !fast;
      if (aces) {
        toneMapper["contrast"] = 1.6773f;
        toneMapper["shoulder"] = 0.9714f;
        toneMapper["midIn"] = 0.18f;
        toneMapper["midOut"] = 0.18f;
        toneMapper["hdrMax"] = 11.0785f;
        toneMapper["acesColor"] = true;
      } else if (filmic) {
        toneMapper["contrast"] = 1.1759f;
        toneMapper["shoulder"] = 0.9746f;
        toneMapper["midIn"] = 0.18f;
        toneMapper["midOut"] = 0.18f;
        toneMapper["hdrMax"] = 6.3704f;
        toneMapper["acesColor"] = false;
      }
      fb2.setChild("toneMapper", toneMapper.shared_from_this());
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

        std::string importString;
        for (auto file : animatedFile) {
          FileName fn = file.file;
          if (fn.ext() == "ospsg")
            sg::loadOSPSG(renderer.shared_from_this(), fn.str());
          else {
            importString += file.file + ",";
          }
        }
        importString.erase(importString.end()-1);
        if (importString != "") {
          auto importerNode_ptr = sg::createNode(animatedFile[0].file, "Importer");
          auto &importerNode = *importerNode_ptr;
          importerNode["fileName"] = importString;
          transform.add(importerNode_ptr);
          transform.markAsModified();

          auto transferFunctions = importerNode.childrenRecursive("transferFunction");
          for (auto tf : transferFunctions)
            renderer["transferFunctions"].add(tf);

          if (animatedFile.size() > 1) {
            auto &anim_selector = importerNode.child("selector")["index"].createChild(
                "anim_" + animatedFile[0].file, "Animator");

            anim_selector.createChild("value2", "int", int(animatedFile.size()));
            animation.setChild("anim_selector", anim_selector.shared_from_this());
          }
        }
      }
    }

    void OSPApp::addPlaneToScene(sg::Node &renderer)
    {
      if (addPlane == false) {
        return;
      }
      auto &world = renderer["world"];
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
    }

  } // ::ospray::app
} // ::ospray
