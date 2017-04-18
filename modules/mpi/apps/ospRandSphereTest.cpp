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

#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"

#include "common/commandline/CameraParser.h"

#include "widgets/imguiViewer.h"

namespace ospRandSphereTest {

  extern "C" int main(int ac, const char **av)
  {
    auto device = ospray::cpp::Device("mpi_distributed");

    //TODO: set any device parameters?

    device.commit();
    device.setCurrent();

    ospray::imgui3D::init(&ac,av);

    DefaultCameraParser cameraClParser;
    cameraClParser.parse(ac, av);
    auto camera = cameraClParser.camera();

    ospray::cpp::Renderer renderer("raycast");
    renderer.commit();

    std::deque<ospcommon::box3f>   bbox;
    std::deque<ospray::cpp::Model> model;

    // TODO: create a random set of spheres based on rank

#if 0
    ospray::ImGuiViewer window(bbox, model, renderer, camera);
    window.create("OSPRay Random Spheres Test App");

    ospray::imgui3D::run();
#endif

    return 0;
  }

} // ::ospRandSphereTest
