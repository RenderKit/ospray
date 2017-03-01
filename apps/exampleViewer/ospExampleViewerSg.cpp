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

#include "sg/geometry/TriangleMesh.h"

#include "widgets/imguiViewerSg.h"

#include <sstream>

using namespace ospcommon;
using namespace ospray;

std::vector<std::string> files;
bool addPlane = true;
bool debug = false;

void parseFilesFromCommandLine(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-np" || arg == "--no-plane") {
      addPlane = false;
    } else if (arg == "-d" || arg == "--debug") {
      debug = true;
    } else {
      FileName fileName = std::string(av[i]);
      auto ext = fileName.ext();
      if (ext == "obj" || ext == "osg" || ext == "ply" || ext == "osp")
        files.push_back(av[i]);
    }
  }
}

void addPlaneToScene(sg::NodeH &world)
{
  //add plane
  auto bbox = world->getBounds();

  osp::vec3f *vertices = new osp::vec3f[4];
  float ps = bbox.upper.x*5.f;
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
  auto plane = sg::createNode("plane", "InstanceGroup");
  auto mesh  = sg::createNode("mesh", "TriangleMesh");
  std::shared_ptr<sg::TriangleMesh> sg_plane =
    std::static_pointer_cast<sg::TriangleMesh>(mesh.get());
  sg_plane->vertex = position;
  sg_plane->index = index;
  auto &planeMaterial = mesh["material"];
  planeMaterial["Kd"]->setValue(vec3f(0.5f));
  planeMaterial["Ks"]->setValue(vec3f(0.6f));
  planeMaterial["Ns"]->setValue(2.f);
  plane += mesh;
  world += plane;
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

  parseFilesFromCommandLine(ac, av);

  sg::RenderContext ctx;
  auto root = sg::createNode("ospray");

  auto renderer = sg::createNode("renderer", "Renderer");
  renderer["shadowsEnabled"]->setValue(true);
  renderer["aoSamples"]->setValue(1);
  renderer["camera"]["fovy"]->setValue(60.f);
  root->add(renderer);

  auto &lights = renderer["lights"];

  auto sun =  sg::createNode("sun", "DirectionalLight");
  sun["color"]->setValue(vec3f(1.f,232.f/255.f,166.f/255.f));
  sun["direction"]->setValue(vec3f(0.462f,-1.f,-.1f));
  sun["intensity"]->setValue(1.5f);
  lights += sun;

  auto bounce = sg::createNode("bounce", "DirectionalLight");
  bounce["color"]->setValue(vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
  bounce["direction"]->setValue(vec3f(-.93,-.54f,-.605f));
  bounce["intensity"]->setValue(0.25f);
  lights += bounce;

  auto ambient = sg::createNode("ambient", "AmbientLight");
  ambient["intensity"]->setValue(0.9f);
  ambient["color"]->setValue(vec3f(174.f/255.f,218.f/255.f,255.f/255.f));
  lights += ambient;

  auto &world = renderer["world"];

  for (auto file : files) {
    FileName fn = file;
    auto ext = fn.ext();
    auto importerNode = sg::createNode(fn.name(), "Importer");
    importerNode["fileName"]->setValue(fn.str());
    world += importerNode;
  }

  if (debug) {
    root->traverse(ctx, "verify");
    root->traverse(ctx, "print");
  }

  renderer->traverse(ctx, "commit");
  renderer->traverse(ctx, "render");

  ospray::ImGuiViewerSg window(renderer);
  if (addPlane) addPlaneToScene(world);
  window.create("OSPRay Example Viewer App");

  ospray::imgui3D::run();
}
