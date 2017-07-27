// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "ospcommon/vec.h"
#include "common/sg/common/Animator.h"

#include "sg/geometry/TriangleMesh.h"

#include "widgets/imguiViewerSg.h"
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
std::vector< std::vector<clFile> > animatedFiles;
std::string initialRendererType;
bool addPlane = true;
bool debug = false;
bool fullscreen = false;
bool print = false;
bool no_defaults = false;
std::string hdri_light;

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

      if (addNode)
      {
        std::stringstream vals(value);
        std::string name, type;
        vals >> name >> type;
        try {
          node.createChild(name, type);
        } catch (const std::runtime_error &) {
          std::cerr << "Warning: unknown sg::Node type '" << type
            << "', ignoring option '" << orgarg << "'." << std::endl;
        }
        continue;
      }
      //Carson: TODO: reimplement with a way of determining type of node value
      //  currently relies on exception on value cast
      try {
        node.valueAs<std::string>();
        node.setValue(value);
      } catch(...) {}
      try {
        node.valueAs<float>();
        std::stringstream vals(value);
        float x;
        vals >> x;
        node.setValue(x);
      } catch(...) {}
      try {
        node.valueAs<int>();
        std::stringstream vals(value);
        int x;
        vals >> x;
        node.setValue(x);
      } catch(...) {}
      try {
        node.valueAs<bool>();
        std::stringstream vals(value);
        bool x;
        vals >> x;
        node.setValue(x);
      } catch(...) {}
      try {
        node.valueAs<ospcommon::vec3f>();
        std::stringstream vals(value);
        float x,y,z;
        vals >> x >> y >> z;
        node.setValue(ospcommon::vec3f(x,y,z));
      } catch(...) {}
      try {
        node.valueAs<ospcommon::vec2i>();
        std::stringstream vals(value);
        int x,y;
        vals >> x >> y;
        node.setValue(ospcommon::vec2i(x,y));
      } catch(...) {}
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

  osp::vec3f *vertices = new osp::vec3f[4];
  float ps = bbox.upper.x*3.f;
  float py = bbox.lower.z-.1f;

  py = bbox.lower.y+0.01f;
  vertices[0] = osp::vec3f{-ps, py, -ps};
  vertices[1] = osp::vec3f{-ps, py, ps};
  vertices[2] = osp::vec3f{ps, py, -ps};
  vertices[3] = osp::vec3f{ps, py, ps};
  auto position = std::make_shared<sg::DataArray3f>((vec3f*)&vertices[0],
                                                    size_t(4),
                                                    false);
  position->setName("vertex");

  osp::vec3i *triangles = new osp::vec3i[2];
  triangles[0] = osp::vec3i{0,1,2};
  triangles[1] = osp::vec3i{1,2,3};
  auto index = std::make_shared<sg::DataArray3i>((vec3i*)&triangles[0],
                                                 size_t(2),
                                                 false);
  index->setName("index");

  auto &plane = world.createChild("plane", "Instance");
  plane["visible"].setValue(addPlane);
  auto &mesh  = plane.child("model").createChild("mesh", "TriangleMesh");

  auto sg_plane = mesh.nodeAs<sg::TriangleMesh>();
  sg_plane->add(position);
  sg_plane->add(index);
  auto &planeMaterial = mesh["material"];
  planeMaterial["Kd"].setValue(vec3f(0.5f));
  planeMaterial["Ks"].setValue(vec3f(0.1f));
  planeMaterial["Ns"].setValue(10.f);
}

static inline void addLightsToScene(sg::Node& renderer)
{
  auto &lights = renderer["lights"];

  if (!no_defaults)
  {
    auto &sun = lights.createChild("sun", "DirectionalLight");
    sun["color"].setValue(vec3f(1.f,232.f/255.f,166.f/255.f));
    sun["direction"].setValue(vec3f(0.462f,-1.f,-.1f));
    sun["intensity"].setValue(1.5f);

    auto &bounce = lights.createChild("bounce", "DirectionalLight");
    bounce["color"].setValue(vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
    bounce["direction"].setValue(vec3f(-.93,-.54f,-.605f));
    bounce["intensity"].setValue(0.25f);

    if (hdri_light == "")
    {
      auto &ambient = lights.createChild("ambient", "AmbientLight");
      ambient["intensity"].setValue(0.9f);
      ambient["color"].setValue(vec3f(174.f/255.f,218.f/255.f,255.f/255.f));
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
      auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
      auto &importerNode = *importerNode_ptr;
      importerNode["fileName"].setValue(fn.str());
      auto &transform = world.createChild("transform_"+file.file, "Transform");
      transform["scale"].setValue(file.transform.scale);
      transform["position"].setValue(file.transform.translate);
      transform["rotation"].setValue(file.transform.rotation);
      if (files.size() < 2 && animatedFiles.empty()) {
        auto &rotation =
          transform["rotation"].createChild("animator", "Animator");

        rotation.traverse("verify");
        rotation.traverse("commit");
        rotation.child("value1").setValue(ospcommon::vec3f{0.f,0.f,0.f});
        rotation.child("value2").setValue(ospcommon::vec3f{0.f,2.f*3.14f,0.f});

        animation.setChild("rotation", rotation.shared_from_this());
      }

      transform += importerNode_ptr;
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

    transform["scale"].setValue(animatedFile[0].transform.scale);
    transform["position"].setValue(animatedFile[0].transform.translate);
    transform["rotation"].setValue(animatedFile[0].transform.rotation);
    auto &selector =
      transform.createChild("selector_" + animatedFile[0].file, "Selector");

    for (auto file : animatedFile) {
      FileName fn = file.file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer.shared_from_this(),fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"].setValue(fn.str());
        selector += importerNode_ptr;
      }
    }
    auto& anim_selector =
      selector["index"].createChild("anim_"+animatedFile[0].file, "Animator");

    anim_selector.traverse("verify");
    anim_selector.traverse("commit");
    anim_selector["value2"].setValue(int(animatedFile.size()));
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
  ospDeviceSetStatusFunc(device,
                         [](const char *msg) { std::cout << msg; });

  ospDeviceSetErrorFunc(device,
                        [](OSPError e, const char *msg) {
                          std::cout << "OSPRAY ERROR [" << e << "]: "
                                    << msg << std::endl;
                          std::exit(1);
                        });

#ifdef _WIN32
  // TODO: Why do we not have the sg symbols already available for us
  // since we link against it?
  loadLibrary("ospray_sg");
#endif

  ospray::imgui3D::init(&ac,av);

  parseCommandLine(ac, av);

  auto renderer_ptr = sg::createNode("renderer", "Renderer");
  auto &renderer = *renderer_ptr;

  auto &win_size = ospray::imgui3D::ImGui3DWidget::defaultInitSize;
  renderer["frameBuffer"]["size"].setValue(win_size);

  if (!initialRendererType.empty())
    renderer["rendererType"].setValue(initialRendererType);

  renderer.createChild("animationcontroller", "AnimationController");

  addLightsToScene(renderer);
  addImporterNodesToWorld(renderer);
  addAnimatedImporterNodesToWorld(renderer);  
  if (!no_defaults)
    addPlaneToScene(renderer);

  // last, to be able to modify all created SG nodes
  parseCommandLineSG(ac, av, renderer);

  if (print || debug)
    renderer.traverse("print");

  ospray::ImGuiViewerSg window(renderer_ptr);

  auto &viewPort = window.viewPort;
  // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
  auto dir = normalize(viewPort.at - viewPort.from);
  renderer["camera"]["dir"].setValue(dir);
  renderer["camera"]["pos"].setValue(viewPort.from);
  renderer["camera"]["up"].setValue(viewPort.up);

  window.create("OSPRay Example Viewer App", fullscreen);

  ospray::imgui3D::run();
}

