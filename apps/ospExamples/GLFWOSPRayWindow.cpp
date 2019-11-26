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
#include "imgui_impl_glfw_gl3.h"
// ospray_testing
#include "ospray_testing.h"
// imgui
#include "imgui.h"
// std
#include <iostream>
#include <stdexcept>

static bool g_quitNextFrame = false;

static const std::vector<std::string> g_scenes = {"boxes",
                                                  "cornell_box",
                                                  "curves",
                                                  "gravity_spheres_volume",
                                                  "perlin_noise_volumes",
                                                  "random_spheres",
                                                  "streamlines",
                                                  "subdivision_cube",
                                                  "unstructured_volume"};

static const std::vector<std::string> g_curveBasis = {"bspline",
                                                      "hermite",
                                                      "catmull-rom",
                                                      "linear"};

static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "raycast"};

bool sceneUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_scenes[index].c_str();
  return true;
}

bool curveBasisUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_curveBasis[index].c_str();
  return true;
}


bool rendererUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_renderers[index].c_str();
  return true;
}

// GLFWOSPRayWindow definitions ///////////////////////////////////////////////

GLFWOSPRayWindow *GLFWOSPRayWindow::activeWindow = nullptr;

GLFWOSPRayWindow::GLFWOSPRayWindow(const vec2i &windowSize)
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

          auto res = w.framebuffer.pick(w.renderer, w.camera, w.world, pos);

          if (res.hasHit) {
            std::cout << "Picked geometry [inst: " << res.instance
                      << ", model: " << res.model << ", prim: " << res.primID
                      << "]" << std::endl;
          }
        }
      });

  // OSPRay setup //

  refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);

  commitOutstandingHandles();
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
}

void GLFWOSPRayWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  // create new frame buffer
  framebuffer = cpp::FrameBuffer(
      windowSize,
      OSP_FB_SRGBA,
      OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM | OSP_FB_ALBEDO);
  framebuffer.commit();

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  camera.setParam("aspect", windowSize.x / float(windowSize.y));
  camera.commit();
}

void GLFWOSPRayWindow::updateCamera()
{
  camera.setParam("aspect", windowSize.x / float(windowSize.y));
  camera.setParam("position", arcballCamera->eyePos());
  camera.setParam("direction", arcballCamera->lookDir());
  camera.setParam("up", arcballCamera->upDir());
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
      if (cancelFrameOnInteraction) {
        currentFrame.cancel();
        waitOnOSPRayFrame();
      }
      updateCamera();
      addObjectToCommit(camera.handle());
    }
  }

  previousMouse = mouse;
}

void GLFWOSPRayWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (showUi)
    buildUI();

  if (displayCallback)
    displayCallback(this);

  updateTitleBar();

  static bool firstFrame = true;
  if (firstFrame || currentFrame.isReady()) {
    // display frame rate in window title
    auto displayEnd = std::chrono::high_resolution_clock::now();
    auto durationMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
                                                              displayStart);

    latestFPS = 1000.f / float(durationMilliseconds.count());

    // map OSPRay frame buffer, update OpenGL texture with its contents, then
    // unmap

    waitOnOSPRayFrame();

    auto *fb = framebuffer.map(showAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR);

    const GLint glFormat = showAlbedo ? GL_RGB : GL_RGBA;
    const GLenum glType  = showAlbedo ? GL_FLOAT : GL_UNSIGNED_BYTE;
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

    framebuffer.unmap(fb);

    commitOutstandingHandles();

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
  currentFrame = framebuffer.renderFrame(renderer, camera, world);
}

void GLFWOSPRayWindow::waitOnOSPRayFrame()
{
  currentFrame.wait();
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
    float progress = currentFrame.progress();
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

void GLFWOSPRayWindow::buildUI()
{
  ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
  ImGui::Begin("press 'g' to hide/show UI", nullptr, flags);

  static int whichScene = 0;
  if (ImGui::Combo("scene##whichScene",
                   &whichScene,
                   sceneUI_callback,
                   nullptr,
                   g_scenes.size())) {
    scene = g_scenes[whichScene];
    refreshScene(true);
  }

  if(scene == "curves") {
    static int whichCurveBasis = 0;
    if (ImGui::Combo("curveBasis##whichCurveBasis",
                     &whichCurveBasis,
                     curveBasisUI_callback,
                     nullptr,
                     g_curveBasis.size())) {
      curveBasis = g_curveBasis[whichCurveBasis];
      refreshScene(true);
    }
  }

  static int whichRenderer = 0;
  if (ImGui::Combo("renderer##whichRenderer",
                   &whichRenderer,
                   rendererUI_callback,
                   nullptr,
                   g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];
    if (rendererTypeStr == "scivis")
      rendererType = OSPRayRendererType::SCIVIS;
    else if (rendererTypeStr == "pathtracer")
      rendererType = OSPRayRendererType::PATHTRACER;
    else
      rendererType = OSPRayRendererType::OTHER;
    refreshScene();
  }

  ImGui::Checkbox("cancel frame on interaction", &cancelFrameOnInteraction);
  ImGui::Checkbox("show albedo", &showAlbedo);

  ImGui::Separator();

  static int spp = 1;
  if (ImGui::SliderInt("spp", &spp, 1, 64)) {
    renderer.setParam("spp", spp);
    addObjectToCommit(renderer.handle());
  }

  if (rendererType == OSPRayRendererType::PATHTRACER) {
    static int maxDepth = 20;
    if (ImGui::SliderInt("maxDepth", &maxDepth, 1, 64)) {
      renderer.setParam("maxDepth", maxDepth);
      addObjectToCommit(renderer.handle());
    }

    static int rouletteDepth = 1;
    if (ImGui::SliderInt("rouletteDepth", &rouletteDepth, 1, 64)) {
      renderer.setParam("rouletteDepth", rouletteDepth);
      addObjectToCommit(renderer.handle());
    }

    static float minContribution = 0.001f;
    if (ImGui::SliderFloat("minContribution", &minContribution, 0.f, 1.f)) {
      renderer.setParam("minContribution", minContribution);
      addObjectToCommit(renderer.handle());
    }
  } else if (rendererType == OSPRayRendererType::SCIVIS) {
    static vec3f bgColor(0.f);
    if (ImGui::ColorEdit3("bgColor", bgColor)) {
      renderer.setParam("bgColor", bgColor);
      addObjectToCommit(renderer.handle());
    }

    static int aoSamples = 1;
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64)) {
      renderer.setParam("aoSamples", aoSamples);
      addObjectToCommit(renderer.handle());
    }

    static float aoIntensity = 1.f;
    if (ImGui::SliderFloat("aoIntensity", &aoIntensity, 0.f, 1.f)) {
      renderer.setParam("aoIntensity", aoIntensity);
      addObjectToCommit(renderer.handle());
    }
  }

  if (uiCallback) {
    ImGui::Separator();
    uiCallback();
  }

  ImGui::End();
}

void GLFWOSPRayWindow::commitOutstandingHandles()
{
  auto handles = objectsToCommit.consume();
  if (!handles.empty()) {
    for (auto &h : handles)
      ospCommit(h);
    framebuffer.resetAccumulation();
  }
}

void GLFWOSPRayWindow::refreshScene(bool resetCamera)
{
  auto builder = testing::newBuilder(scene);
  testing::setParam(builder, "rendererType", rendererTypeStr);
  if(scene == "curves") {
    testing::setParam(builder, "curveBasis", curveBasis);
  }
  testing::commit(builder);

  world = testing::buildWorld(builder);
  testing::release(builder);

  world.commit();

  renderer = cpp::Renderer(rendererTypeStr);
  addObjectToCommit(renderer.handle());

  if (resetCamera) {
    // create the arcball camera model
    arcballCamera.reset(new ArcballCamera(world.getBounds(), windowSize));

    // init camera
    updateCamera();
    camera.commit();
  }
}