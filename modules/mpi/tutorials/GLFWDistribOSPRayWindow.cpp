// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GLFWDistribOSPRayWindow.h"
#include <imgui.h>
#include <mpi.h>
#include <ospray/ospray_util.h>
#include <iostream>
#include <stdexcept>
#include "imgui_impl_glfw_gl3.h"

using namespace ospray;
using namespace rkcommon::math;

GLFWDistribOSPRayWindow *GLFWDistribOSPRayWindow::activeWindow = nullptr;

static bool g_quitNextFrame = false;

WindowState::WindowState()
    : quit(false), cameraChanged(false), fbSizeChanged(false), spp(1)
{}

GLFWDistribOSPRayWindow::GLFWDistribOSPRayWindow(const vec2i &windowSize,
    const box3f &worldBounds,
    cpp::World world,
    cpp::Renderer renderer)
    : windowSize(windowSize),
      worldBounds(worldBounds),
      world(world),
      renderer(renderer)
{
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  if (mpiRank == 0) {
    if (activeWindow != nullptr) {
      throw std::runtime_error(
          "Cannot create more than one GLFWDistribOSPRayWindow!");
    }

    activeWindow = this;

    // initialize GLFW
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW!");
    }

    // create GLFW window
    glfwWindow = glfwCreateWindow(
        windowSize.x, windowSize.y, "OSPRay Tutorial", NULL, NULL);

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
      if (!io.WantCaptureMouse) {
        activeWindow->motion(vec2f{float(x), float(y)});
      }
    });

    glfwSetKeyCallback(
        glfwWindow, [](GLFWwindow *, int key, int, int action, int) {
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
  }

  // OSPRay setup

  // create the arcball camera model
  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  // create camera
  camera = cpp::Camera("perspective");
  camera.setParam("aspect", windowSize.x / float(windowSize.y));
  camera.setParam("position", arcballCamera->eyePos());
  camera.setParam("direction", arcballCamera->lookDir());
  camera.setParam("up", arcballCamera->upDir());
  camera.commit();

  renderer.commit();

  windowState.windowSize = windowSize;
  windowState.eyePos = arcballCamera->eyePos();
  windowState.lookDir = arcballCamera->lookDir();
  windowState.upDir = arcballCamera->upDir();

  if (mpiRank == 0) {
    // trigger window reshape events with current window size
    glfwGetFramebufferSize(
        glfwWindow, &this->windowSize.x, &this->windowSize.y);
    reshape(this->windowSize);
  }
}

GLFWDistribOSPRayWindow::~GLFWDistribOSPRayWindow()
{
  if (mpiRank == 0) {
    ImGui_ImplGlfwGL3_Shutdown();
    // cleanly terminate GLFW
    glfwTerminate();
  }
}

GLFWDistribOSPRayWindow *GLFWDistribOSPRayWindow::getActiveWindow()
{
  return activeWindow;
}

cpp::World GLFWDistribOSPRayWindow::getWorld()
{
  return world;
}

void GLFWDistribOSPRayWindow::setWorld(cpp::World newWorld)
{
  world = newWorld;
  addObjectToCommit(world.handle());
}

void GLFWDistribOSPRayWindow::resetAccumulation()
{
  framebuffer.resetAccumulation();
}

void GLFWDistribOSPRayWindow::registerDisplayCallback(
    std::function<void(GLFWDistribOSPRayWindow *)> callback)
{
  displayCallback = callback;
}

void GLFWDistribOSPRayWindow::registerImGuiCallback(
    std::function<void()> callback)
{
  uiCallback = callback;
}

void GLFWDistribOSPRayWindow::mainLoop()
{
  while (true) {
    MPI_Bcast(&windowState, sizeof(WindowState), MPI_BYTE, 0, MPI_COMM_WORLD);
    if (windowState.quit) {
      break;
    }

    // TODO: Actually render asynchronously, if we have MPI thread multiple
    // support
    startNewOSPRayFrame();
    waitOnOSPRayFrame();

    // if a display callback has been registered, call it
    if (displayCallback) {
      displayCallback(this);
    }

    if (mpiRank == 0) {
      ImGui_ImplGlfwGL3_NewFrame();

      display();

      // poll and process events
      glfwPollEvents();
      windowState.quit = glfwWindowShouldClose(glfwWindow) || g_quitNextFrame;
    }
  }
}

void GLFWDistribOSPRayWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;
  windowState.windowSize = windowSize;
  windowState.fbSizeChanged = true;

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);
}

void GLFWDistribOSPRayWindow::motion(const vec2f &position)
{
  static vec2f previousMouse(-1);

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
      const vec2f mouseFrom(clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
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
      windowState.cameraChanged = true;
      windowState.eyePos = arcballCamera->eyePos();
      windowState.lookDir = arcballCamera->lookDir();
      windowState.upDir = arcballCamera->upDir();
    }
  }

  previousMouse = mouse;
}

void GLFWDistribOSPRayWindow::display()
{
  // clock used to compute frame rate
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (showUi && uiCallback) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(
        "Tutorial Controls (press 'g' to hide / show)", nullptr, flags);
    uiCallback();

    ImGui::End();
  }

  updateTitleBar();

  static bool firstFrame = true;
  if (firstFrame || currentFrame.isReady()) {
    // display frame rate in window title
    auto displayEnd = std::chrono::high_resolution_clock::now();
    auto durationMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            displayEnd - displayStart);

    latestFPS = 1000.f / float(durationMilliseconds.count());

    // map OSPRay frame buffer, update OpenGL texture with its contents, then
    // unmap

    uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        windowSize.x,
        windowSize.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        fb);

    framebuffer.unmap(fb);

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
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

  ImGui::Render();

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void GLFWDistribOSPRayWindow::startNewOSPRayFrame()
{
  bool fbNeedsClear = false;
  auto handles = objectsToCommit.consume();
  if (!handles.empty()) {
    for (auto &h : handles)
      ospCommit(h);

    fbNeedsClear = true;
  }

  if (windowState.fbSizeChanged) {
    windowState.fbSizeChanged = false;
    windowSize = windowState.windowSize;

    framebuffer = cpp::FrameBuffer(
        windowSize.x, windowSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
    camera.setParam("aspect", windowSize.x / float(windowSize.y));
    camera.commit();
    fbNeedsClear = true;
  }
  if (windowState.cameraChanged) {
    windowState.cameraChanged = false;
    camera.setParam("aspect", windowSize.x / float(windowSize.y));
    camera.setParam("position", windowState.eyePos);
    camera.setParam("direction", windowState.lookDir);
    camera.setParam("up", windowState.upDir);
    camera.commit();
    fbNeedsClear = true;
  }

  if (fbNeedsClear) {
    framebuffer.resetAccumulation();
  }

  currentFrame = framebuffer.renderFrame(renderer, camera, world);
}

void GLFWDistribOSPRayWindow::waitOnOSPRayFrame()
{
  if (currentFrame.handle() != nullptr) {
    currentFrame.wait(OSP_FRAME_FINISHED);
  }
}

void GLFWDistribOSPRayWindow::addObjectToCommit(OSPObject obj)
{
  objectsToCommit.push_back(obj);
}

void GLFWDistribOSPRayWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay: " << std::setprecision(3) << latestFPS << " fps";
  if (latestFPS < 2.f) {
    float progress = currentFrame.progress();
    windowTitle << " | ";
    int barWidth = 20;
    std::string progBar;
    progBar.resize(barWidth + 2);
    auto start = progBar.begin() + 1;
    auto end = start + progress * barWidth;
    std::fill(start, end, '=');
    std::fill(end, progBar.end(), '_');
    *end = '>';
    progBar.front() = '[';
    progBar.back() = ']';
    windowTitle << progBar;
  }

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}
