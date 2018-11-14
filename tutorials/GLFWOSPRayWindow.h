// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include "ArcballCamera.h"
#include "ospcommon/box.h"
#include "ospcommon/vec.h"
#include "ospray/ospray.h"

class GLFWOSPRayWindow
{
 public:
  GLFWOSPRayWindow(const ospcommon::vec2i &windowSize,
                   const ospcommon::box3f &worldBounds,
                   OSPModel model,
                   OSPRenderer renderer);

  ~GLFWOSPRayWindow();

  OSPModel getModel();
  void setModel(OSPModel newModel);

  void registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)> callback);

  void mainLoop();

 protected:
  void reshape(const ospcommon::vec2i &newWindowSize);
  void motion(const ospcommon::vec2f &position);
  void display();

  static GLFWOSPRayWindow *activeWindow;

  ospcommon::vec2i windowSize;
  ospcommon::box3f worldBounds;
  OSPModel model;
  OSPRenderer renderer;

  // GLFW window instance
  GLFWwindow *glfwWindow;

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;

  // OSPRay objects managed by this class
  OSPCamera camera;
  OSPFrameBuffer framebuffer;

  // OpenGL framebuffer texture
  GLuint framebufferTexture;

  // optional registered display callback, called before every display()
  std::function<void(GLFWOSPRayWindow *)> displayCallback;
};
