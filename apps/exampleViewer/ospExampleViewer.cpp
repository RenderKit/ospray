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

#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"
#include "common/sg/importer/Importer.h"
#include "ospcommon/FileName.h"
#include "ospcommon/networking/Socket.h"
#include "ospcommon/utility/getEnvVar.h"
#include "ospcommon/vec.h"
#include "common/sg/common/Animator.h"

#include "sg/geometry/TriangleMesh.h"

#include "widgets/imguiViewer.h"
#include <sstream>

using namespace ospcommon;
using namespace ospray;

struct clTransform
{
  vec3f translate{0,0,0};
  vec3f scale{1,1,1};
  vec3f rotation{0,0,0};
};

//command line file
struct clFile
{
  clFile(std::string f, const clTransform& t) : file(f), transform(t) {}
  std::string file;
  clTransform transform;
};

std::vector<clFile> files;
std::vector<std::vector<clFile>> animatedFiles;
std::string initialRendererType;

bool addPlane =
    utility::getEnvVar<int>("OSPRAY_VIEWER_GROUND_PLANE").value_or(true);

bool debug = false;
bool fullscreen = false;
bool print = false;
bool no_defaults = false;
std::string hdri_light;
int matrix_i =1, matrix_j=1, matrix_k = 1;

static inline void parseCommandLine(int ac, const char **&av)
{
  clTransform currentCLTransform;
  bool inAnimation = false;
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-np" || arg == "--no-plane") {
      addPlane = false;
    } else if (arg == "-d" || arg == "--debug") {
      debug = true;
    } else if (arg == "-r" || arg == "--renderer") {
      initialRendererType = av[++i];
    } else if (arg == "-m" || arg == "--module") {
      ospLoadModule(av[++i]);
    } else if (arg == "--print") {
      print=true;
    } else if (arg == "--no-defaults") {
      no_defaults=true;
    } else if (arg == "--matrix") {
      matrix_i = atoi(av[++i]);
      matrix_j = atoi(av[++i]);
      matrix_k = atoi(av[++i]);
    } else if (arg == "--fullscreen") {
      fullscreen = true;
    } else if (arg == "--hdri-light") {
      hdri_light = av[++i];
    } else if (arg == "--translate") {
      currentCLTransform.translate.x = atof(av[++i]);
      currentCLTransform.translate.y = atof(av[++i]);
      currentCLTransform.translate.z = atof(av[++i]);
    } else if (arg == "--scale") {
      currentCLTransform.scale.x = atof(av[++i]);
      currentCLTransform.scale.y = atof(av[++i]);
      currentCLTransform.scale.z = atof(av[++i]);
    } else if (arg == "--rotate") {
      currentCLTransform.rotation.x = atof(av[++i]);
      currentCLTransform.rotation.y = atof(av[++i]);
      currentCLTransform.rotation.z = atof(av[++i]);
    } else if (arg == "--animation") {
      inAnimation = true;
      animatedFiles.push_back(std::vector<clFile>());
    } else if (arg == "--file") {
      inAnimation = false;
    } else if (arg[0] != '-') {
      if (!inAnimation)
        files.push_back(clFile(av[i], currentCLTransform));
      else
        animatedFiles.back().push_back(clFile(av[i], currentCLTransform));
    }
  }
}

//parse command line arguments containing the format:
//  -nodeName:...:nodeName=value,value,value -- changes value
//  -nodeName:...:nodeName+=name,type        -- adds new child node
static inline void parseCommandLineSG(int ac, const char **&av, sg::Node &root)
{
  for(int i=1;i < ac; i++) {
    std::string arg(av[i]);
    size_t f;
    std::string value("");
    if (arg.size() < 2 || arg[0] != '-')
      continue;

    const std::string orgarg(arg);
    while ((f = arg.find(":")) != std::string::npos ||
           (f = arg.find(",")) != std::string::npos) {
      arg[f] = ' ';
    }

    f = arg.find("+=");
    bool addNode = false;
    if (f != std::string::npos)
    {
      value = arg.substr(f+2,arg.size());
      addNode = true;
    }
    else
    {
      f = arg.find("=");
      if (f != std::string::npos)
        value = arg.substr(f+1,arg.size());
    }

    if (value != "") {
      std::stringstream ss;
      ss << arg.substr(1,f-1);
      std::string child;
      std::reference_wrapper<sg::Node> node_ref = root;
      try {
        while (ss >> child) {
          node_ref = node_ref.get().childRecursive(child);
        }
      } catch (const std::runtime_error &) {
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
        } catch (const std::runtime_error &) {
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
          float x,y,z;
          vals >> x >> y >> z;
          node.setValue(ospcommon::vec3f(x,y,z));
        } else if (node.valueIsType<ospcommon::vec2i>()) {
          int x,y;
          vals >> x >> y;
          node.setValue(ospcommon::vec2i(x,y));
        } else try {
          auto &vec = dynamic_cast<sg::DataVector1f&>(node);
          float f;
          while (vals.good()) {
            vals >> f;
            vec.push_back(f);
          }
        } catch(...) {}
      }
    }
  }
}

static inline void addPlaneToScene(sg::Node& renderer)
{
  auto &world = renderer["world"];

  // update world bounds first
  renderer.traverse("verify");
  renderer.traverse("commit");

  auto bbox = world.bounds();
  if (bbox.empty()) {
    bbox.lower = vec3f(-5,0,-5);
    bbox.upper = vec3f(5,10,5);
  }

  float ps = bbox.upper.x * 3.f;
  float py = bbox.lower.y + 0.01f;

  auto position = std::make_shared<sg::DataVector3f>();
  position->push_back(vec3f{-ps, py, -ps});
  position->push_back(vec3f{-ps, py, ps});
  position->push_back(vec3f{ps, py, -ps});
  position->push_back(vec3f{ps, py, ps});
  position->setName("vertex");

  auto index = std::make_shared<sg::DataVector3i>();
  index->push_back(vec3i{0,1,2});
  index->push_back(vec3i{1,2,3});
  index->setName("index");

  auto &plane = world.createChild("plane", "TriangleMesh");

  auto sg_plane = plane.nodeAs<sg::TriangleMesh>();
  sg_plane->add(position);
  sg_plane->add(index);
  auto &planeMaterial = plane["material"];
  planeMaterial["Kd"] = vec3f(0.5f);
  planeMaterial["Ks"] = vec3f(0.1f);
  planeMaterial["Ns"] = 10.f;
}

static inline void addLightsToScene(sg::Node& renderer)
{
  auto &lights = renderer["lights"];

  if (!no_defaults)
  {
    auto &sun = lights.createChild("sun", "DirectionalLight");
    sun["color"] = vec3f(1.f,232.f/255.f,166.f/255.f);
    sun["direction"] = vec3f(0.462f,-1.f,-.1f);
    sun["intensity"] = 1.5f;

    auto &bounce = lights.createChild("bounce", "DirectionalLight");
    bounce["color"] = vec3f(127.f/255.f,178.f/255.f,255.f/255.f);
    bounce["direction"] = vec3f(-.93,-.54f,-.605f);
    bounce["intensity"] = 0.25f;

    if (hdri_light == "")
    {
      auto &ambient = lights.createChild("ambient", "AmbientLight");
      ambient["intensity"] = 0.9f;
      ambient["color"] = vec3f(174.f/255.f,218.f/255.f,255.f/255.f);
    }
  }

  if (hdri_light != "")
  {
    auto tex = sg::Texture2D::load(hdri_light, false);
    tex->setName("map");
    auto& hdri = lights.createChild("hdri", "HDRILight");
    tex->traverse("verify");
    tex->traverse("commit");
    hdri.add(tex);
  }

}

static inline void addImporterNodesToWorld(sg::Node& renderer)
{
  auto &world = renderer["world"];
  auto &animation = renderer["animationcontroller"];

  for (auto file : files) {
    FileName fn = file.file;
    if (fn.ext() == "ospsg")
      sg::loadOSPSG(renderer.shared_from_this(), fn.str());
    else {
      //create material array
      for (int i=0;i<matrix_i;i++)
      {
        for(int j=0;j<matrix_j;j++)
        {
          for(int k=0;k<matrix_k;k++)
          {
            std::stringstream ss;
            ss << fn.name() << "_" << i << "_" << j << "_" << k;
            auto importerNode_ptr = sg::createNode(ss.str(), "Importer")->nodeAs<sg::Importer>();;
            auto &importerNode = *importerNode_ptr;

            auto &transform = world.createChild("transform_"+ss.str(), "Transform");
            transform.add(importerNode_ptr);
            importerNode["fileName"] = fn.str();

            transform["scale"] = file.transform.scale;
            transform["rotation"] = file.transform.rotation;
            if (files.size() < 2 && animatedFiles.empty()) {
              auto &rotation = transform["rotation"].createChild("animator", "Animator");

              rotation.traverse("verify");
              rotation.traverse("commit");
              rotation.child("value1") = vec3f(0.f,0.f,0.f);
              rotation.child("value2") = vec3f(0.f,2.f*3.14f,0.f);

              animation.setChild("rotation", rotation.shared_from_this());
            }

            renderer.traverse("verify");
            renderer.traverse("commit");
            auto bounds = importerNode_ptr->computeBounds();
            auto size = bounds.upper - bounds.lower;
            float maxSize = max(max(size.x,size.y),size.z);
            vec3f offset={i*maxSize*1.3f,j*maxSize*1.3f,k*maxSize*1.3f};
            transform["position"] = file.transform.translate + offset;
          }
        }
      }
    }
  }
}

static inline void addAnimatedImporterNodesToWorld(sg::Node& renderer)
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
      transform.createChild("selector_" + animatedFile[0].file, "Selector");

    for (auto file : animatedFile) {
      FileName fn = file.file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer.shared_from_this(),fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"] = fn.str();
        selector.add(importerNode_ptr);
      }
    }
    auto& anim_selector =
      selector["index"].createChild("anim_"+animatedFile[0].file, "Animator");

    anim_selector.traverse("verify");
    anim_selector.traverse("commit");
    anim_selector["value2"] = int(animatedFile.size());
    animation.setChild("anim_selector", anim_selector.shared_from_this());
  }
}

int main(int ac, const char **av)
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
                        });

  ospDeviceCommit(device);

  // access/load symbols/sg::Nodes dynamically
  loadLibrary("ospray_sg");

  ospray::imgui3D::init(&ac,av);

  parseCommandLine(ac, av);

  auto renderer_ptr = sg::createNode("renderer", "Renderer");
  auto &renderer = *renderer_ptr;

  auto &win_size = ospray::imgui3D::ImGui3DWidget::defaultInitSize;
  renderer["frameBuffer"]["size"] = win_size;

  if (!initialRendererType.empty())
    renderer["rendererType"] = initialRendererType;

  renderer.createChild("animationcontroller", "AnimationController");

  addLightsToScene(renderer);
  addImporterNodesToWorld(renderer);
  addAnimatedImporterNodesToWorld(renderer);
  if (!no_defaults && addPlane)
    addPlaneToScene(renderer);

  // last, to be able to modify all created SG nodes
  parseCommandLineSG(ac, av, renderer);

  if (print || debug)
    renderer.traverse("print");

  ospray::ImGuiViewer window(renderer_ptr);

  auto &viewPort = window.viewPort;
  // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
  auto dir = normalize(viewPort.at - viewPort.from);
  renderer["camera"]["dir"] = dir;
  renderer["camera"]["pos"] = viewPort.from;
  renderer["camera"]["up"]  = viewPort.up;
  if (renderer["camera"].hasChild("fovy"))
    renderer["camera"]["fovy"] = viewPort.openingAngle;
  if (renderer["camera"].hasChild("apertureRadius"))
    renderer["camera"]["apertureRadius"] = viewPort.apertureRadius;
  if (renderer["camera"].hasChild("focusdistance"))
    renderer["camera"]["focusdistance"] = length(viewPort.at - viewPort.from);

  window.create("OSPRay Example Viewer App", fullscreen);

  ospray::imgui3D::run();
  return 0;
}

