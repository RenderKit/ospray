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
#include "tutorial_util.h"
// glfw
#include "GLFW/glfw3.h"
// ospcommon
#include "ospcommon/containers/TransactionalBuffer.h"
// std
#include <functional>

class GLFWOSPRayWindow
{
 public:
  GLFWOSPRayWindow(const vec2i &windowSize,
                   const box3f &worldBounds,
                   OSPWorld world,
                   OSPRenderer renderer,
                   OSPFrameBufferFormat fbFormat = OSP_FB_SRGBA,
                   uint32_t fbChannels           = OSP_FB_COLOR | OSP_FB_DEPTH |
                                         OSP_FB_ACCUM | OSP_FB_ALBEDO);

  ~GLFWOSPRayWindow();

  static GLFWOSPRayWindow *getActiveWindow();

  OSPWorld getWorld();
  void setWorld(OSPWorld newWorld);

  void setImageOps(OSPData ops);

  void resetAccumulation();

  void registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)> callback);

  void registerImGuiCallback(std::function<void()> callback);

  void mainLoop();

  void updateCamera();
  void commitCamera();

  void addObjectToCommit(OSPObject obj);

  std::unique_ptr<ArcballCamera> &getArcballCamera()
  {
    return arcballCamera;
  }

 protected:
  void reshape(const vec2i &newWindowSize);
  void motion(const vec2f &position);
  void display();
  void startNewOSPRayFrame();
  void waitOnOSPRayFrame();
  void updateTitleBar();

  static GLFWOSPRayWindow *activeWindow;

  vec2i windowSize;
  vec2f previousMouse{-1.f};
  box3f worldBounds;
  OSPWorld world       = nullptr;
  OSPRenderer renderer = nullptr;

  bool showAlbedo{false};
  OSPFrameBufferFormat fbFormat;
  uint32_t fbChannels;

  // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;

  // OSPRay objects managed by this class
  OSPCamera camera           = nullptr;
  OSPFrameBuffer framebuffer = nullptr;
  OSPFuture currentFrame     = nullptr;
  OSPData imageOps           = nullptr;

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
