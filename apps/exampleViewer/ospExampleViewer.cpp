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
#include "common/sg/visitor/PrintNodes.h"
#include "ospcommon/utility/getEnvVar.h"
#include "sg/geometry/TriangleMesh.h"

#include "ospapp/OSPApp.h"
#include "widgets/imguiViewer.h"

namespace ospray {
namespace app {
	class OSPExampleViewer : public OSPApp {
	public:
		int main(int ac, const char **av);
	private:
		int initializeOSPRay(int argc, const char *argv[]);
		void addPlaneToScene(sg::Node &renderer);
		int parseCommandLine(int &ac, const char **&av);
		bool debug = false;
		bool fullscreen = false;
		bool print = false;
		bool noDefaults = false;
		bool addPlane = false;
		float motionSpeed = -1.f;
		std::string initialTextForNodeSearch;
	};
}
}

using namespace ospray;

int app::OSPExampleViewer::main(int ac, const char **av) {
  int initResult = initializeOSPRay(ac, av);
  if (initResult != 0)
  	return initResult;

  // access/load symbols/sg::Nodes dynamically
  loadLibrary("ospray_sg");


  if(parseCommandLine(ac, av) != 0) {
	return 1;
  }

  imgui3D::init(pos, up, gaze, fovy, apertureRadius, width, height);

  auto renderer_ptr = sg::createNode("renderer", "Renderer");
  auto &renderer = *renderer_ptr;

  renderer["frameBuffer"]["size"] = imgui3D::ImGui3DWidget::defaultInitSize;

  if (!initialRendererType.empty())
    renderer["rendererType"] = initialRendererType;

  renderer.createChild("animationcontroller", "AnimationController");

  addLightsToScene(renderer, !noDefaults);
  addImporterNodesToWorld(renderer);
  addAnimatedImporterNodesToWorld(renderer);
  if (!noDefaults && addPlane)
    addPlaneToScene(renderer);

  // last, to be able to modify all created SG nodes
  parseCommandLineSG(ac, av, renderer);

  if (print || debug)
    renderer.traverse(sg::PrintNodes{});

  ospray::ImGuiViewer window(renderer_ptr);

  auto &viewPort = window.viewPort;
  // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
  auto dir = normalize(viewPort.at - viewPort.from);
  renderer["camera"]["dir"] = dir;
  renderer["camera"]["pos"] = viewPort.from;
  renderer["camera"]["up"] = viewPort.up;
  if (renderer["camera"].hasChild("fovy"))
    renderer["camera"]["fovy"] = viewPort.openingAngle;
  if (renderer["camera"].hasChild("apertureRadius"))
    renderer["camera"]["apertureRadius"] = viewPort.apertureRadius;
  if (renderer["camera"].hasChild("focusdistance"))
    renderer["camera"]["focusdistance"] = length(viewPort.at - viewPort.from);

  window.create("OSPRay Example Viewer App", fullscreen);

  if (motionSpeed > 0.f)
    window.setMotionSpeed(motionSpeed);

  if (!initialTextForNodeSearch.empty())
    window.setInitialSearchBoxText(initialTextForNodeSearch);

  imgui3D::run();
  return 0;
}

int app::OSPExampleViewer::initializeOSPRay(int argc, const char *argv[]) {
	addPlane = utility::getEnvVar<int>("OSPRAY_VIEWER_GROUND_PLANE").value_or(true);
	return ospray::app::OSPApp::initializeOSPRay(argc, argv);

}

void ospray::app::OSPExampleViewer::addPlaneToScene(sg::Node &renderer) {
  auto &world = renderer["world"];

  // update world bounds first
  renderer.traverse("verify");
  renderer.traverse("commit");

  auto bbox = world.bounds();
  if (bbox.empty()) {
    bbox.lower = vec3f(-5, 0, -5);
    bbox.upper = vec3f(5, 10, 5);
  }

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
  auto &planeMaterial = (*plane["materialList"].nodeAs<sg::MaterialList>())[0];
  planeMaterial["Kd"] = vec3f(0.5f);
  planeMaterial["Ks"] = vec3f(0.1f);
  planeMaterial["Ns"] = 10.f;
}

int app::OSPExampleViewer::parseCommandLine(int &ac, const char **&av) {
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-np" || arg == "--no-plane") {
      addPlane = false;
	removeArgs(ac,av,i,1); --i;
    } else  if (arg == "-d" || arg == "--debug") {
      debug = true;
	removeArgs(ac,av,i,1); --i;
    } else if (arg == "--print") {
      print = true;
	removeArgs(ac,av,i,1); --i;
    } else if (arg == "--no-defaults") {
      noDefaults = true;
	removeArgs(ac,av,i,1); --i;
    } else if (arg == "--fullscreen") {
      fullscreen = true;
	removeArgs(ac,av,i,1); --i;
    } else if (arg == "--motionSpeed") {
      motionSpeed = atof(av[i+1]);
	removeArgs(ac,av,i,2); --i;
    } else if (arg == "--searchText") {
      initialTextForNodeSearch = av[i+1];
	removeArgs(ac,av,i,2); --i;
    }
  }
  return app::OSPApp::parseCommandLine(ac, av);
}


int main(int ac, const char **av) {
  app::OSPExampleViewer ospApp;
  return ospApp.main(ac, av);
}
