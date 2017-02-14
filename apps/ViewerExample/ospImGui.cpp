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

#include <ospray/ospray_cpp/Device.h>
#include "common/commandline/Utility.h"
#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"
#include "common/sg/importer/Importer.h"
#include "ospcommon/FileName.h"

#include "sg/geometry/TriangleMesh.h"

#include "widgets/imguiViewer.h"

#include <sstream>

using namespace ospray;

ospcommon::vec3f translate;
ospcommon::vec3f scale;
bool lockFirstFrame = false;
std::vector<std::string> files;
bool showGui = false;

void parseExtraParametersFromComandLine(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--translate") {
      translate.x = atof(av[++i]);
      translate.y = atof(av[++i]);
      translate.z = atof(av[++i]);
    } else if (arg == "--scale") {
      scale.x = atof(av[++i]);
      scale.y = atof(av[++i]);
      scale.z = atof(av[++i]);
    } else if (arg == "--lockFirstFrame") {
      lockFirstFrame = true;
    } else if (arg == "--nogui") {
      showGui = false;
    }
    else //look for valid models
    {
      ospcommon::FileName fileName = std::string(av[i]);
      if (fileName.ext() == "obj" ||
        fileName.ext() == "osg" ||
        fileName.ext() == "ply"
        )
      {
        files.push_back(av[i]);
      }
    }
  }
}

void parseCommandLine(int ac, const char **&av, sg::NodeH root)
{
  for(int i=1;i < ac; i++)
  {
    std::string arg(av[i]);
    size_t f;
    std::string value("");
    std::cout << "parsing cl arg: \"" << arg <<"\"" << std::endl;
    while ( (f = arg.find(":")) != std::string::npos || (f = arg.find(",")) != std::string::npos)
      arg[f]=' ';
    f = arg.find("=");
    if (f != std::string::npos)
    {
      value = arg.substr(f+1,arg.size());
      std::cout << "found value: " << value << std::endl;
    }
    if (value != "")
    {
      std::stringstream ss;
      ss << arg.substr(0,f);
      std::string child;
      sg::NodeH node = root;
      while (ss >> child)
      {
        std::cout << "searching for node: " << child << std::endl;
        node = node->getChildRecursive(child);
        std::cout << "found node: " << node->getName() << std::endl;
      }
      try
      {
        node->getValue<std::string>();
        node->setValue(value);
      } catch(...) {};
      try
      {
        std::stringstream vals(value);
        float x;
        vals >> x;
        node->getValue<float>();
        node->setValue(x);
      } catch(...) {}
      try
      {
        std::stringstream vals(value);
        int x;
        vals >> x;
        node->getValue<int>();
        node->setValue(x);
      } catch(...) {}
      try
      {
        std::stringstream vals(value);
        bool x;
        vals >> x;
        node->getValue<bool>();
        node->setValue(x);
      } catch(...) {}
      try
      {
        std::stringstream vals(value);
        float x,y,z;
        vals >> x >> y >> z;
        node->getValue<ospcommon::vec3f>();
        node->setValue(ospcommon::vec3f(x,y,z));
      } catch(...) {}
      try
      {
        std::stringstream vals(value);
        int x,y,z;
        vals >> x >> y;
        node->getValue<ospcommon::vec2i>();
        node->setValue(ospcommon::vec2i(x,y));
      } catch(...) {}
    }
  }
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

  auto ospObjs = parseWithDefaultParsers(ac, av);

  std::deque<ospcommon::box3f>   bbox;
  std::deque<ospray::cpp::Model> model;
  ospray::cpp::Renderer renderer;
  ospray::cpp::Camera   camera;

  std::tie(bbox, model, renderer, camera) = ospObjs;

  parseExtraParametersFromComandLine(ac, av);
  ospray::imgui3D::ImGui3DWidget::showGui = showGui;

  sg::RenderContext ctx;
  sg::NodeH root = sg::createNode("ospray");
  std::cout << root->getName() << std::endl;
  sg::NodeH rendererN = sg::createNode("renderer", "Renderer");
  root->add(rendererN);
  sg::NodeH sun =  sg::createNode("sun", "DirectionalLight");
  root["renderer"]["lights"] += sun;
  root["renderer"]["lights"]["sun"]["color"]->setValue(ospcommon::vec3f(1.f,232.f/255.f,166.f/255.f));
  root["renderer"]["lights"]["sun"]["direction"]->setValue(ospcommon::vec3f(0.462f,-1.f,-.1f));
  root["renderer"]["lights"]["sun"]["intensity"]->setValue(1.5f);
  root["renderer"]["lights"] += sg::createNode("bounce", "DirectionalLight");
  root["renderer"]["lights"]["bounce"]["color"]->setValue(ospcommon::vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
  root["renderer"]["lights"]["bounce"]["direction"]->setValue(ospcommon::vec3f(-.93,-.54f,-.605f));
  root["renderer"]["lights"]["bounce"]["intensity"]->setValue(0.25f);
  root["renderer"]["lights"] += sg::createNode("ambient", "AmbientLight");
  root["renderer"]["lights"]["ambient"]["intensity"]->setValue(0.9f);
  root["renderer"]["lights"]["ambient"]["color"]->setValue(ospcommon::vec3f(174.f/255.f,218.f/255.f,255.f/255.f));
  root["renderer"]["shadowsEnabled"]->setValue(true);
  root["renderer"]["aoSamples"]->setValue(1);
  root["renderer"]["camera"]["fovy"]->setValue(60.f);

  parseCommandLine(ac, av, root);

  std::shared_ptr<sg::World> world = std::static_pointer_cast<sg::World>(root["renderer"]["world"].get());
  for (auto file : files)
  {
    ospcommon::FileName fn = file;
    if (fn.ext() == "obj")
    {
      // sg::importOBJ(world, file);
      root["renderer"]["world"] += sg::createNode(fn.name(), "Importer");
      root["renderer"]["world"][fn.name()]["fileName"]->setValue(fn.str());
    }
    if (fn.ext() == "osg")
    {
      // root["renderer"]["world"] += createNode(file, "TriangleMesh");
      // root["renderer"]["world"][file]["fileName"]->setValue(file.str());
      // sg::NodeH osgH;
      // std::shared_ptr<sg::World> osg = sg::loadOSG(file);
      // osg->setName("world");
      // root["renderer"]["world"] = sg::NodeH(osg);
      root["renderer"]["world"] += sg::createNode(fn.name(), "Importer");
      root["renderer"]["world"][fn.name()]["fileName"]->setValue(fn.str());
    }
    if (fn.ext() == "ply")
    {
      root["renderer"]["world"] += sg::createNode(fn.name(), "Importer");
      root["renderer"]["world"][fn.name()]["fileName"]->setValue(fn.str());
      // sg::NodeH osgH;
      // sg::NodeH osg(root["renderer"]["world"]);
      // std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(osg.get()));
      // sg::importPLY(wsg, file);
      // osg->setName("world");
      // osg->setType("world");
      // root["renderer"]["world"] = sg::NodeH(osg.get());
    }
  }

  bbox.push_back(std::static_pointer_cast<sg::World>(root["renderer"]["world"].get())->getBounds());
  ospcommon::box3f bounds = std::static_pointer_cast<sg::World>(root["renderer"]["world"].get())->getBounds();
  if (bbox[0].empty())
  {
    bbox[0].lower = vec3f(0.f);
    bbox[0].upper = vec3f(1.f);
  }

  //add plane
  osp::vec3f *vertices = new osp::vec3f[4];
  float ps = bbox[0].upper.x*5.f;
  float py = bbox[0].lower.z-0.01f;
  // vertices[0] = osp::vec3f{-ps, -ps, py};
  // vertices[1] = osp::vec3f{-ps,  ps, py};
  // vertices[2] = osp::vec3f{ ps, -ps, py};
  // vertices[3] = osp::vec3f{ ps,  ps, py};

  py = bbox[0].lower.y-0.01f;
  vertices[0] = osp::vec3f{-ps, py, -ps};
  vertices[1] = osp::vec3f{-ps, py, ps};
  vertices[2] = osp::vec3f{ps, py, -ps};
  vertices[3] = osp::vec3f{ps, py, ps};
  auto position = std::shared_ptr<sg::DataBuffer>(new sg::DataArray3f((ospcommon::vec3f*)&vertices[0],size_t(4),false));
  osp::vec3i *triangles = new osp::vec3i[2];
  triangles[0] = osp::vec3i{0,1,2};
  triangles[1] = osp::vec3i{1,2,3};
  auto index = std::shared_ptr<sg::DataBuffer>(new sg::DataArray3i((ospcommon::vec3i*)&triangles[0],size_t(2),false));
  root["renderer"]["world"] += sg::createNode("plane", "InstanceGroup");
  root["renderer"]["world"]["plane"] += sg::createNode("mesh", "TriangleMesh");
  std::shared_ptr<sg::TriangleMesh> plane =
    std::static_pointer_cast<sg::TriangleMesh>(root["renderer"]["world"]["plane"]["mesh"].get());
  plane->vertex = position;
  plane->index = index;
  root["renderer"]["world"]["plane"]["mesh"]["material"]["Kd"]->setValue(ospcommon::vec3f(0.8f));
  root["renderer"]["world"]["plane"]["mesh"]["material"]["Ks"]->setValue(ospcommon::vec3f(0.6f));
  root["renderer"]["world"]["plane"]["mesh"]["material"]["Ns"]->setValue(2.f);

  root->traverse(ctx, "verify");
  root->traverse(ctx,"print");
  rendererN->traverse(ctx, "commit");
  rendererN->traverse(ctx, "render");

  ospray::ImGuiViewer window(bbox, model, renderer, camera, root["renderer"]);
  window.setScale(scale);
  window.setLockFirstAnimationFrame(lockFirstFrame);
  window.setTranslation(translate);
  window.create("ospImGui: OSPRay ImGui Viewer App");

  ospray::imgui3D::run();
}
