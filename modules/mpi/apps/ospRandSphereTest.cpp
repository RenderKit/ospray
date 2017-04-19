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

// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"
// ospray apps
#include "common/commandline/CameraParser.h"
#include "widgets/imguiViewer.h"
// stl
#include <random>

namespace ospRandSphereTest {

  using namespace ospcommon;

  int   numSpheresPerNode = 100;
  float sphereRadius      = 0.01f;

  struct Sphere
  {
    vec3f org;
    int colorID {0};
  };

  std::pair<ospray::cpp::Model, box3f> makeSpheres()
  {
    box3f bbox;

    std::vector<Sphere> spheres(numSpheresPerNode);

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    for (auto &s : spheres) {
      s.org.x = dist(rng);
      s.org.y = dist(rng);
      s.org.z = dist(rng);
      bbox.extend(s.org);
    }

    ospray::cpp::Data sphere_data(numSpheresPerNode * sizeof(Sphere),
                                  OSP_UCHAR, spheres.data());

    vec4f color(1.f, 0.f, 0.f, 1.f);
    ospray::cpp::Data color_data(1, OSP_FLOAT4, &color);

    ospray::cpp::Geometry geom("spheres");

    geom.set("spheres", sphere_data);
    geom.set("color", color_data);
    geom.set("offset_colorID", int(sizeof(vec3f)));
    geom.set("radius", sphereRadius);
    geom.commit();

    ospray::cpp::Model model;
    model.addGeometry(geom);
    model.commit();

    return std::make_pair(model, bbox);
  }

  void initialize_ospray()
  {
    ospLoadModule("mpi");
    auto device = ospray::cpp::Device("mpi_distributed");
    device.set("masterRank", 0);
    device.commit();
    device.setCurrent();

    ospDeviceSetErrorMsgFunc(device.handle(),
                             [](const char *msg) {
                               std::cerr << msg << std::endl;
                             });
  }

  extern "C" int main(int ac, const char **av)
  {
    initialize_ospray();

    DefaultCameraParser cameraClParser;
    cameraClParser.parse(ac, av);
    auto camera = cameraClParser.camera();

    ospray::cpp::Renderer renderer("raycast");
    renderer.commit();

    auto scene = makeSpheres();

    if (ospray::mpi::IamTheMaster()) {
      ospray::imgui3D::init(&ac,av);

      ospray::ImGuiViewer window({scene.second}, {scene.first},
                                 renderer, camera);
      window.create("OSPRay Random Spheres Test App");

      ospray::imgui3D::run();
    } else {
      //TODO: constantly call ospRenderFrame()?
    }

    return 0;
  }

} // ::ospRandSphereTest
