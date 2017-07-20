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

namespace dw {

  struct ServiceInfo {
    /* constructor that initializes everything to default values */
    ServiceInfo()
      : totalPixelsInWall(-1,-1),
        mpiPortName("<value not set>")
    {}

    /*! total pixels in the entire display wall, across all
      indvididual displays, and including bezels (future versios
      will allow to render to smaller resolutions, too - and have
      the clients upscale this - but for now the client(s) have to
      render at exactly this resolution */
    ospcommon::vec2i totalPixelsInWall;

    /*! the MPI port name that the service is listening on client
      connections for (ie, the one to use with
      client::establishConnection) */
    std::string mpiPortName;

    /*! whether this runs in stereo mode */
    int stereo;

    /*! read a service info from a given hostName:port. The service
      has to already be running on that port

      Note this may throw a std::runtime_error if the connection
      cannot be established
    */
    void getFrom(const std::string &hostName,
                 const int portNo);
  };
  /*! read a service info from a given hostName:port. The service
    has to already be running on that port */
  void ServiceInfo::getFrom(const std::string &hostName,
                            const int portNo)
  {
    ospcommon::socket_t sock = ospcommon::connect(hostName.c_str(),portNo);
    if (!sock)
      throw std::runtime_error("could not create display wall connection!");

    mpiPortName = read_string(sock);
    totalPixelsInWall.x = read_int(sock);
    totalPixelsInWall.y = read_int(sock);
    stereo = read_int(sock);
    close(sock);
  }
}


using namespace ospcommon;
using namespace ospray;

struct clTransform
{
 vec3f translate;
 vec3f scale{1,1,1};
 vec3f rotation;
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
bool addPlane = false;
bool debug = false;
bool fullscreen = false;
bool print = false;


void parseCommandLine(int ac, const char **&av)
{
  clTransform currentCLTransform;
  bool inAnimation = false;
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-p" || arg == "--plane") {
      addPlane = true;
    } else if (arg == "-d" || arg == "--debug") {
      debug = true;
    } else if (arg == "-r" || arg == "--renderer") {
      initialRendererType = av[++i];
    } else if (arg == "-m" || arg == "--module") {
      ospLoadModule(av[++i]);
    } else if (arg == "--print") {
      print=true;
    } else if (arg == "--fullscreen") {
      fullscreen = true;
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
//  -nodeName:...:nodeName=value,value,value
void parseCommandLineSG(int ac, const char **&av, sg::Node &root)
{
  for(int i=1;i < ac; i++) {
    std::string arg(av[i]);
    size_t f;
    std::string value("");
    if (arg.size() < 2 || arg[0] != '-')
      continue;

    while ((f = arg.find(":")) != std::string::npos ||
           (f = arg.find(",")) != std::string::npos) {
      arg[f] = ' ';
    }

    f = arg.find("=");
    if (f != std::string::npos)
      value = arg.substr(f+1,arg.size());

    if (value != "") {
      std::stringstream ss;
      ss << arg.substr(1,f-1);
      std::string child;
      std::reference_wrapper<sg::Node> node_ref = root;
      while (ss >> child) {
        node_ref = node_ref.get().childRecursive(child);
      }
      auto &node = node_ref.get();
      //Carson: TODO: reimplement with a way of determining type of node value
      //  currently relies on exception on value cast
      try {
        node.valueAs<std::string>();
        node.setValue(value);
      } catch(...) {};
      try {
        std::stringstream vals(value);
        float x;
        vals >> x;
        node.valueAs<float>();
        node.setValue(x);
      } catch(...) {}
      try {
        std::stringstream vals(value);
        int x;
        vals >> x;
        node.valueAs<int>();
        node.setValue(x);
      } catch(...) {}
      try {
        std::stringstream vals(value);
        bool x;
        vals >> x;
        node.valueAs<bool>();
        node.setValue(x);
      } catch(...) {}
      try {
        std::stringstream vals(value);
        float x,y,z;
        vals >> x >> y >> z;
        node.valueAs<ospcommon::vec3f>();
        node.setValue(ospcommon::vec3f(x,y,z));
      } catch(...) {}
      try {
        std::stringstream vals(value);
        int x,y;
        vals >> x >> y;
        node.valueAs<ospcommon::vec2i>();
        node.setValue(ospcommon::vec2i(x,y));
      } catch(...) {}
    }
  }
}

void addPlaneToScene(sg::Node& world)
{
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
  planeMaterial["Ks"].setValue(vec3f(0.6f));
  planeMaterial["Ns"].setValue(2.f);
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
  /*! the renderer we use for rendering on the display wall; null if
      no dw available */
  std::shared_ptr<sg::Node> rendererDW;
  /*! display wall service info - ignore if 'rendererDW' is null */
  dw::ServiceInfo dwService;

  const char *dwNodeName = getenv("DISPLAY_WALL");
  if (dwNodeName) {
    std::cout << "#######################################################"
              << std::endl;
    std::cout << "found a DISPLAY_WALL environment variable ...." << std::endl;
    std::cout << "trying to connect to display wall service on "
              << dwNodeName << ":2903" << std::endl;

    dwService.getFrom(dwNodeName,2903);
    std::cout << "found display wall service on MPI port "
              << dwService.mpiPortName << std::endl;
    std::cout << "#######################################################"
              << std::endl;
    rendererDW = sg::createNode("renderer", "Renderer");
  }

  renderer["shadowsEnabled"].setValue(true);
  renderer["aoSamples"].setValue(1);
  renderer["camera"]["fovy"].setValue(60.f);

  if (rendererDW.get()) {
    rendererDW->child("shadowsEnabled").setValue(true);
    rendererDW->child("aoSamples").setValue(1);
    rendererDW->child("camera")["fovy"].setValue(60.f);
  }

  if (!initialRendererType.empty()) {
    renderer["rendererType"].setValue(initialRendererType);
    if (rendererDW.get()) {
      rendererDW->child("rendererType").setValue(initialRendererType);
    }
  }

  auto &lights = renderer["lights"];

  auto &sun = lights.createChild("sun", "DirectionalLight");
  sun["color"].setValue(vec3f(1.f,232.f/255.f,166.f/255.f));
  sun["direction"].setValue(vec3f(0.462f,-1.f,-.1f));
  sun["intensity"].setValue(1.5f);


  auto &bounce = lights.createChild("bounce", "DirectionalLight");
  bounce["color"].setValue(vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
  bounce["direction"].setValue(vec3f(-.93,-.54f,-.605f));
  bounce["intensity"].setValue(0.25f);

  auto &ambient = lights.createChild("ambient", "AmbientLight");
  ambient["intensity"].setValue(0.9f);
  ambient["color"].setValue(vec3f(174.f/255.f,218.f/255.f,255.f/255.f));

  auto &world = renderer["world"];


  auto &animation = renderer.createChild("animationcontroller", "AnimationController");

  for (auto file : files) {
    FileName fn = file.file;
    if (fn.ext() == "ospsg")
      sg::loadOSPSG(renderer_ptr,fn.str());
    else {
      auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
      auto &importerNode = *importerNode_ptr;
      importerNode["fileName"].setValue(fn.str());
      if (files.size() < 2 && importerNode.hasChildRecursive("rotation"))
      {
        auto &rotation = importerNode.childRecursive("rotation").createChild("animator", "Animator");
        rotation.traverse("verify");
        rotation.traverse("commit");
        rotation.child("value1").setValue(ospcommon::vec3f{0.f,0.f,0.f});
        rotation.child("value2").setValue(ospcommon::vec3f{0.f,2.f*3.14f,0.f});
        animation.setChild("rotation", rotation.shared_from_this());
      }
      world += importerNode_ptr;
    }
  }
  for (auto animatedFile : animatedFiles)
  {
    if (animatedFile.empty())
      continue;
    auto &transform = world.createChild("transform_"+animatedFile[0].file, "Transform");
    transform["scale"].setValue(animatedFile[0].transform.scale);
    transform["position"].setValue(animatedFile[0].transform.translate);
    transform["rotation"].setValue(animatedFile[0].transform.rotation);
    auto &selector = transform.createChild("selector_"+animatedFile[0].file, "Selector");
    for (auto file : animatedFile)
    {
      FileName fn = file.file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer_ptr,fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"].setValue(fn.str());
        selector += importerNode_ptr;
      }
    }
    auto& anim_selector = selector["index"].createChild("anim_"+animatedFile[0].file, "Animator");
    anim_selector.traverse("verify");
    anim_selector.traverse("commit");
    anim_selector["value2"].setValue(int(animatedFile.size()));
    animation.setChild("anim_selector", anim_selector.shared_from_this());
  }

  parseCommandLineSG(ac, av, renderer);

  if (rendererDW.get()) {
    rendererDW->setChild("world",  renderer["world"].shared_from_this());
    rendererDW->setChild("lights", renderer["lights"].shared_from_this());

    auto &frameBuffer = rendererDW->child("frameBuffer");
    frameBuffer["size"].setValue(dwService.totalPixelsInWall);
    frameBuffer["displayWall"].setValue(dwService.mpiPortName);
  }

  if (print || debug)
    renderer.traverse("print");

  ospray::ImGuiViewerSg window(renderer_ptr, rendererDW);

  addPlaneToScene(world);

  window.create("OSPRay Example Viewer App", fullscreen);

  ospray::imgui3D::run();
}

