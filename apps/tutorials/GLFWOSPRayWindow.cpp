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

#include "GLFWOSPRayWindow.h"
#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include "imgui/imgui_impl_glfw_gl3.h"

GLFWOSPRayWindow *GLFWOSPRayWindow::activeWindow = nullptr;

static bool g_quitNextFrame = false;

GLFWOSPRayWindow::GLFWOSPRayWindow(const ospcommon::vec2i &windowSize,
                                   const ospcommon::box3f &worldBounds,
                                   OSPWorld world,
                                   OSPRenderer renderer)
    : windowSize(windowSize),
      worldBounds(worldBounds),
      world(world),
      renderer(renderer)
{
  if (activeWindow != nullptr) {
    throw std::runtime_error("Cannot create more than one GLFWOSPRayWindow!");
  }

  activeWindow = this;

  // initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Tutorial", nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  // make the window's context current
  glfwMakeContextCurrent(glfwWindow);

  ImGui_ImplGlfwGL3_Init(glfwWindow, true);

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        activeWindow->reshape(ospcommon::vec2i{newWidth, newHeight});
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      activeWindow->motion(ospcommon::vec2f{float(x), float(y)});
    }
  });

  glfwSetKeyCallback(glfwWindow,
                     [](GLFWwindow *, int key, int, int action, int) {
                       if (action == GLFW_PRESS) {
                         switch (key) {
                         case GLFW_KEY_G:
                           activeWindow->showUi = !(activeWindow->showUi);
                           break;
                         case GLFW_KEY_Q:
                           g_quitNextFrame = true;
                           break;
                         }
                       }
                     });

  glfwSetMouseButtonCallback(
      glfwWindow, [](GLFWwindow *, int button, int action, int /*mods*/) {
        auto &w = *activeWindow;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
          auto mouse      = activeWindow->previousMouse;
          auto windowSize = activeWindow->windowSize;
          const ospcommon::vec2f pos(
              mouse.x / static_cast<float>(windowSize.x),
              1.f - mouse.y / static_cast<float>(windowSize.y));

          OSPPickResult res;
          ospPick(&res,
                  w.framebuffer,
                  w.renderer,
                  w.camera,
                  w.world,
                  (const osp_vec2f &)pos);

          if (res.hasHit) {
            std::cout << "Hit geometry instance [id: " << res.geometryInstance
                      << ", prim: " << res.primID << "]" << std::endl;
          }
        }
      });

  // OSPRay setup //

  // create the arcball camera model
  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  // create camera
  camera = ospNewCamera("perspective");
  ospSet1f(camera, "aspect", windowSize.x / float(windowSize.y));

  ospSet3f(camera,
           "pos",
           arcballCamera->eyePos().x,
           arcballCamera->eyePos().y,
           arcballCamera->eyePos().z);
  ospSet3f(camera,
           "dir",
           arcballCamera->lookDir().x,
           arcballCamera->lookDir().y,
           arcballCamera->lookDir().z);
  ospSet3f(camera,
           "up",
           arcballCamera->upDir().x,
           arcballCamera->upDir().y,
           arcballCamera->upDir().z);

  ospCommit(camera);

  // finally, commit the renderer
  ospCommit(renderer);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
}

GLFWOSPRayWindow::~GLFWOSPRayWindow()
{
  ImGui_ImplGlfwGL3_Shutdown();
  // cleanly terminate GLFW
  glfwTerminate();
}

GLFWOSPRayWindow *GLFWOSPRayWindow::getActiveWindow()
{
  return activeWindow;
}

OSPWorld GLFWOSPRayWindow::getWorld()
{
  return world;
}

void GLFWOSPRayWindow::setWorld(OSPWorld newWorld)
{
  world = newWorld;
}

void GLFWOSPRayWindow::setPixelOps(OSPData ops)
{
  pixelOps = ops;
  ospSetData(framebuffer, "pixelOperations", pixelOps);
  addObjectToCommit(framebuffer);
}

void GLFWOSPRayWindow::resetAccumulation()
{
  ospResetAccumulation(framebuffer);
}

void GLFWOSPRayWindow::registerDisplayCallback(
    std::function<void(GLFWOSPRayWindow *)> callback)
{
  displayCallback = callback;
}

void GLFWOSPRayWindow::registerImGuiCallback(std::function<void()> callback)
{
  uiCallback = callback;
}

void GLFWOSPRayWindow::mainLoop()
{
  // continue until the user closes the window
  startNewOSPRayFrame();

  while (!glfwWindowShouldClose(glfwWindow) && !g_quitNextFrame) {
    ImGui_ImplGlfwGL3_NewFrame();

    display();

    // poll and process events
    glfwPollEvents();
  }

  waitOnOSPRayFrame();
  if (currentFrame != nullptr)
    ospRelease(currentFrame);

  ospRelease(camera);
  ospRelease(framebuffer);
  ospRelease(world);
  ospRelease(renderer);
}

void GLFWOSPRayWindow::reshape(const ospcommon::vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  // release the current frame buffer, if it exists
  if (framebuffer) {
    ospRelease(framebuffer);
  }

  // create new frame buffer
  framebuffer = ospNewFrameBuffer(reinterpret_cast<osp_vec2i &>(windowSize),
                                  OSP_FB_SRGBA,
                                  OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_ALBEDO);

  if (pixelOps) {
    ospSetData(framebuffer, "pixelOperations", pixelOps);
  }

  ospCommit(framebuffer);

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  ospSet1f(camera, "aspect", windowSize.x / float(windowSize.y));
  ospCommit(camera);
}

void GLFWOSPRayWindow::motion(const ospcommon::vec2f &position)
{
  const ospcommon::vec2f mouse(position.x, position.y);
  if (previousMouse != ospcommon::vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const ospcommon::vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    if (leftDown) {
      const ospcommon::vec2f mouseFrom(
          ospcommon::clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          ospcommon::clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const ospcommon::vec2f mouseTo(
          ospcommon::clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          ospcommon::clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      arcballCamera->zoom(mouse.y - prev.y);
    } else if (middleDown) {
      arcballCamera->pan(ospcommon::vec2f(mouse.x - prev.x, prev.y - mouse.y));
    }

    if (cameraChanged) {
      ospSet1f(camera, "aspect", windowSize.x / float(windowSize.y));
      ospSet3f(camera,
               "pos",
               arcballCamera->eyePos().x,
               arcballCamera->eyePos().y,
               arcballCamera->eyePos().z);
      ospSet3f(camera,
               "dir",
               arcballCamera->lookDir().x,
               arcballCamera->lookDir().y,
               arcballCamera->lookDir().z);
      ospSet3f(camera,
               "up",
               arcballCamera->upDir().x,
               arcballCamera->upDir().y,
               arcballCamera->upDir().z);

      addObjectToCommit(camera);
    }
  }

  previousMouse = mouse;
}

void GLFWOSPRayWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (showUi) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(
        "Tutorial Controls (press 'g' to hide / show)", nullptr, flags);

    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      addObjectToCommit(renderer);
    }

    ImGui::Checkbox("show albedo", &showAlbedo);

    if (uiCallback) {
      ImGui::Separator();
      uiCallback();
    }
    ImGui::End();
  }

  if (displayCallback) {
    displayCallback(this);
  }

  updateTitleBar();

  static bool firstFrame = true;
  if (firstFrame || ospIsReady(currentFrame)) {
    // display frame rate in window title
    auto displayEnd = std::chrono::high_resolution_clock::now();
    auto durationMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
                                                              displayStart);

    latestFPS = 1000.f / float(durationMilliseconds.count());

    // map OSPRay frame buffer, update OpenGL texture with its contents, then
    // unmap

    waitOnOSPRayFrame();

    auto *fb = ospMapFrameBuffer(framebuffer,
                                 showAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR);

    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 showAlbedo ? GL_RGB : GL_RGBA,
                 windowSize.x,
                 windowSize.y,
                 0,
                 showAlbedo ? GL_RGB : GL_RGBA,
                 showAlbedo ? GL_FLOAT : GL_UNSIGNED_BYTE,
                 fb);

    ospUnmapFrameBuffer(fb, framebuffer);

    auto handles = objectsToCommit.consume();
    if (!handles.empty()) {
      for (auto &h : handles)
        ospCommit(h);
      ospResetAccumulation(framebuffer);
    }

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
    firstFrame = false;
  }

  // clear current OpenGL color buffer
  glClear(GL_COLOR_BUFFER_BIT);

  // render textured quad with OSPRay frame buffer contents
  glBegin(GL_QUADS);

  glTexCoord2f(0.f, 0.f);
  glVertex2f(0.f, 0.f);

  glTexCoord2f(0.f, 1.f);
  glVertex2f(0.f, windowSize.y);

  glTexCoord2f(1.f, 1.f);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f, 0.f);
  glVertex2f(windowSize.x, 0.f);

  glEnd();

  if (showUi) {
    ImGui::Render();
  }

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void GLFWOSPRayWindow::startNewOSPRayFrame()
{
  if (currentFrame != nullptr)
    ospRelease(currentFrame);

  currentFrame = ospRenderFrameAsync(framebuffer, renderer, camera, world);
}

void GLFWOSPRayWindow::waitOnOSPRayFrame()
{
  if (currentFrame != nullptr) {
    ospWait(currentFrame, OSP_FRAME_FINISHED);
  }
}

void GLFWOSPRayWindow::addObjectToCommit(OSPObject obj)
{
  objectsToCommit.push_back(obj);
}

void GLFWOSPRayWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay: " << std::setprecision(3) << latestFPS << " fps";
  if (latestFPS < 2.f) {
    float progress = ospGetProgress(currentFrame);
    windowTitle << " | ";
    int barWidth = 20;
    std::string progBar;
    progBar.resize(barWidth + 2);
    auto start = progBar.begin() + 1;
    auto end   = start + progress * barWidth;
    std::fill(start, end, '=');
    std::fill(end, progBar.end(), '_');
    *end            = '>';
    progBar.front() = '[';
    progBar.back()  = ']';
    windowTitle << progBar;
  }

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}
