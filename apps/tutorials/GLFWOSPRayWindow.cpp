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

GLFWOSPRayWindow::GLFWOSPRayWindow(const vec2i &windowSize,
                                   const box3f &worldBounds,
                                   OSPWorld world,
                                   OSPRenderer renderer,
                                   OSPFrameBufferFormat fbFormat,
                                   uint32_t fbChannels)
    : worldBounds(worldBounds),
      world(world),
      renderer(renderer),
      fbFormat(fbFormat),
      fbChannels(fbChannels)
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
        activeWindow->reshape(vec2i{newWidth, newHeight});
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->motion(vec2f{float(x), float(y)});
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
          const vec2f pos(mouse.x / static_cast<float>(windowSize.x),
                          1.f - mouse.y / static_cast<float>(windowSize.y));

          OSPPickResult res;
          ospPick(
              &res, w.framebuffer, w.renderer, w.camera, w.world, pos.x, pos.y);

          if (res.hasHit) {
            std::cout << "Picked geometry [inst: " << res.instance
                      << ", model: " << res.model << ", prim: " << res.primID
                      << "]" << std::endl;
          }
        }
      });

  // OSPRay setup //

  // create the arcball camera model
  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  // create camera
  camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));
  updateCamera();
  commitCamera();

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

void GLFWOSPRayWindow::setImageOps(OSPData ops)
{
  imageOps = ops;
  ospSetObject(framebuffer, "imageOperation", imageOps);
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

void GLFWOSPRayWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  // release the current frame buffer, if it exists
  if (framebuffer) {
    ospRelease(framebuffer);
  }

  // create new frame buffer
  framebuffer =
      ospNewFrameBuffer(windowSize.x, windowSize.y, fbFormat, fbChannels);

  if (imageOps) {
    ospSetObject(framebuffer, "imageOperation", imageOps);
  }

  ospCommit(framebuffer);

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));
  ospCommit(camera);
}

void GLFWOSPRayWindow::updateCamera()
{
  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));
  ospSetVec3f(camera,
              "position",
              arcballCamera->eyePos().x,
              arcballCamera->eyePos().y,
              arcballCamera->eyePos().z);
  ospSetVec3f(camera,
              "direction",
              arcballCamera->lookDir().x,
              arcballCamera->lookDir().y,
              arcballCamera->lookDir().z);
  ospSetVec3f(camera,
              "up",
              arcballCamera->upDir().x,
              arcballCamera->upDir().y,
              arcballCamera->upDir().z);
}

void GLFWOSPRayWindow::commitCamera()
{
  ospCommit(camera);
}

void GLFWOSPRayWindow::motion(const vec2f &position)
{
  const vec2f mouse(position.x, position.y);
  if (previousMouse != vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    if (leftDown) {
      const vec2f mouseFrom(
          clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
                          clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      arcballCamera->zoom(mouse.y - prev.y);
    } else if (middleDown) {
      arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y));
    }

    if (cameraChanged) {
      updateCamera();
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
      ospSetInt(renderer, "spp", spp);
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

    const GLint glFormat = showAlbedo ? GL_RGB : GL_RGBA;
    const GLenum glType =
        showAlbedo || fbFormat == OSP_FB_RGBA32F ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 glFormat,
                 windowSize.x,
                 windowSize.y,
                 0,
                 glFormat,
                 glType,
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

  currentFrame = ospRenderFrame(framebuffer, renderer, camera, world);
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
