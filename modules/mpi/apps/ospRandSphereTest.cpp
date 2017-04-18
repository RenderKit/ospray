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

#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Device.h>
#include <ospray/ospray_cpp/FrameBuffer.h>
#include <ospray/ospray_cpp/Renderer.h>

#include "widgets/imguiViewer.h"

namespace ospRandSphereTest {

  extern "C" int main(int ac, const char **av)
  {
    // TODO: manually create MPIDistributedDevice and populate parameters

    ospray::imgui3D::init(&ac,av);

    std::deque<ospcommon::box3f>   bbox;
    std::deque<ospray::cpp::Model> model;
    ospray::cpp::Renderer renderer;
    ospray::cpp::Camera   camera;

    // TODO: create a random set of spheres based on rank
    // TODO: create renderer
    // TODO: create camera

    ospray::ImGuiViewer window(bbox, model, renderer, camera);
    window.create("OSPRay Random Spheres Test App");

    ospray::imgui3D::run();

    return 0;
  }

} // ::ospRandSphereTest
