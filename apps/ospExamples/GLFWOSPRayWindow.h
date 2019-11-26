// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

#include "ArcballCamera.h"
// glfw
#include "GLFW/glfw3.h"
// ospray
#include "ospray/ospray_cpp.h"
// ospcommon
#include "ospcommon/containers/TransactionalBuffer.h"
// std
#include <functional>

using namespace ospcommon::math;
using namespace ospray;

enum class OSPRayRendererType
{
  SCIVIS,
  PATHTRACER,
  OTHER
};

class GLFWOSPRayWindow
{
 public:
  GLFWOSPRayWindow(const vec2i &windowSize);

  ~GLFWOSPRayWindow();

  static GLFWOSPRayWindow *getActiveWindow();

  void registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)> callback);

  void registerImGuiCallback(std::function<void()> callback);

  void mainLoop();

  void addObjectToCommit(OSPObject obj);

 protected:
  void updateCamera();

  void reshape(const vec2i &newWindowSize);
  void motion(const vec2f &position);
  void display();
  void startNewOSPRayFrame();
  void waitOnOSPRayFrame();
  void updateTitleBar();
  void buildUI();
  void commitOutstandingHandles();
  void refreshScene(bool resetCamera = false);

  static GLFWOSPRayWindow *activeWindow;

  vec2i windowSize;
  vec2f previousMouse{-1.f};

  bool showAlbedo{false};
  bool cancelFrameOnInteraction{false};

  // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;

  // OSPRay objects managed by this class
  cpp::Renderer renderer;
  cpp::Camera camera{"perspective"};
  cpp::World world;
  cpp::FrameBuffer framebuffer;
  cpp::Future currentFrame;

  std::string scene{"boxes"};

  std::string curveBasis{"bspline"};

  OSPRayRendererType rendererType{OSPRayRendererType::SCIVIS};
  std::string rendererTypeStr{"scivis"};

  // List of OSPRay handles to commit before the next frame
  ospcommon::containers::TransactionalBuffer<OSPObject> objectsToCommit;

  // OpenGL framebuffer texture
  GLuint framebufferTexture = 0;

  // optional registered display callback, called before every display()
  std::function<void(GLFWOSPRayWindow *)> displayCallback;

  // toggles display of ImGui UI, if an ImGui callback is provided
  bool showUi = true;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;

  // FPS measurement of last frame
  float latestFPS{0.f};
};
