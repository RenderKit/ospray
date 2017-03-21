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
#include "ospcommon/Socket.h"
#include "ospcommon/vec.h"

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

std::vector<std::string> files;
std::string initialRendererType;
bool addPlane = true;
bool debug = false;
bool fullscreen = false;

void parseCommandLine(int ac, const char **&av)
{
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
    } else if (arg == "--fullscreen") {
      fullscreen = true;
    } else if (arg[0] != '-') {
      files.push_back(av[i]);
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
  //add plane
  auto bbox = world.bounds();

  osp::vec3f *vertices = new osp::vec3f[4];
  float ps = bbox.upper.x*3.f;
  float py = bbox.lower.z-0.01f;

  py = bbox.lower.y-0.01f;
  vertices[0] = osp::vec3f{-ps, py, -ps};
  vertices[1] = osp::vec3f{-ps, py, ps};
  vertices[2] = osp::vec3f{ps, py, -ps};
  vertices[3] = osp::vec3f{ps, py, ps};
  auto position = std::make_shared<sg::DataArray3f>((vec3f*)&vertices[0],
                                                    size_t(4),
                                                    false);
  osp::vec3i *triangles = new osp::vec3i[2];
  triangles[0] = osp::vec3i{0,1,2};
  triangles[1] = osp::vec3i{1,2,3};
  auto index = std::make_shared<sg::DataArray3i>((vec3i*)&triangles[0],
                                                 size_t(2),
                                                 false);
  auto &plane = world.createChildNode("plane", "InstanceGroup");
  auto &mesh  = plane.createChildNode("mesh", "TriangleMesh");

  std::shared_ptr<sg::TriangleMesh> sg_plane =
    std::static_pointer_cast<sg::TriangleMesh>(mesh.shared_from_this());
  sg_plane->vertex = position;
  sg_plane->index = index;
  auto &planeMaterial = mesh["material"];
  planeMaterial["Kd"].setValue(vec3f(0.5f));
  planeMaterial["Ks"].setValue(vec3f(0.6f));
  planeMaterial["Ns"].setValue(2.f);
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);

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

  auto &sun = lights.createChildNode("sun", "DirectionalLight");
  sun["color"].setValue(vec3f(1.f,232.f/255.f,166.f/255.f));
  sun["direction"].setValue(vec3f(0.462f,-1.f,-.1f));
  sun["intensity"].setValue(1.5f);

  auto &bounce = lights.createChildNode("bounce", "DirectionalLight");
  bounce["color"].setValue(vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
  bounce["direction"].setValue(vec3f(-.93,-.54f,-.605f));
  bounce["intensity"].setValue(0.25f);

  auto &ambient = lights.createChildNode("ambient", "AmbientLight");
  ambient["intensity"].setValue(0.9f);
  ambient["color"].setValue(vec3f(174.f/255.f,218.f/255.f,255.f/255.f));

  auto &world = renderer["world"];

  for (auto file : files) {
    FileName fn = file;
    auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
    auto &importerNode = *importerNode_ptr;
    importerNode["fileName"].setValue(fn.str());
    world += importerNode_ptr;
  }

  parseCommandLineSG(ac, av, renderer);

  if (rendererDW.get()) {
    rendererDW->properties.children["world"]  =
        renderer["world"].shared_from_this();
    rendererDW->properties.children["lights"] =
        renderer["lights"].shared_from_this();

    auto &frameBuffer = rendererDW->child("frameBuffer");
    frameBuffer["size"].setValue(dwService.totalPixelsInWall);
    frameBuffer["displayWall"].setValue(dwService.mpiPortName);
  }

  if (debug) {
    renderer.traverse("verify");
    renderer.traverse("print");
  }

  renderer.traverse("commit");
  
  ospray::ImGuiViewerSg window(renderer, rendererDW);
  if (addPlane) addPlaneToScene(world);
  window.create("OSPRay Example Viewer App", fullscreen);
  
  ospray::imgui3D::run();
}
  
