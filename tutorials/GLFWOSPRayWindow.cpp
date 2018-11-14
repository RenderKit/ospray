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

#include "GLFWOSPRayWindow.h"
#include <iostream>
#include <stdexcept>

GLFWOSPRayWindow *GLFWOSPRayWindow::activeWindow = nullptr;

GLFWOSPRayWindow::GLFWOSPRayWindow(const ospcommon::vec2i &windowSize,
                                   const ospcommon::box3f &worldBounds,
                                   OSPModel model,
                                   OSPRenderer renderer)
    : windowSize(windowSize),
      worldBounds(worldBounds),
      model(model),
      renderer(renderer),
      framebuffer(nullptr)
{
  if (activeWindow != nullptr)
    throw std::runtime_error("Cannot create more than one GLFWOSPRayWindow!");

  activeWindow = this;

  // initialize GLFW
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW!");

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Tutorial", NULL, NULL);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  // make the window's context current
  glfwMakeContextCurrent(glfwWindow);

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
    activeWindow->motion(ospcommon::vec2f{float(x), float(y)});
  });

  // OSPRay setup

  // set the model on the renderer
  ospSetObject(renderer, "model", model);

  // create the arcball camera model
  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  // create camera
  camera = ospNewCamera("perspective");
  ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));

  ospSetVec3f(camera,
              "pos",
              osp::vec3f{arcballCamera->eyePos().x,
                         arcballCamera->eyePos().y,
                         arcballCamera->eyePos().z});
  ospSetVec3f(camera,
              "dir",
              osp::vec3f{arcballCamera->lookDir().x,
                         arcballCamera->lookDir().y,
                         arcballCamera->lookDir().z});
  ospSetVec3f(camera,
              "up",
              osp::vec3f{arcballCamera->upDir().x,
                         arcballCamera->upDir().y,
                         arcballCamera->upDir().z});

  ospCommit(camera);

  // set camera on the renderer
  ospSetObject(renderer, "camera", camera);

  // finally, commit the renderer
  ospCommit(renderer);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
}

GLFWOSPRayWindow::~GLFWOSPRayWindow()
{
  // cleanly terminate GLFW
  glfwTerminate();
}

OSPModel GLFWOSPRayWindow::getModel()
{
  return model;
}

void GLFWOSPRayWindow::setModel(OSPModel newModel)
{
  model = newModel;

  // set the model on the renderer
  ospSetObject(renderer, "model", model);

  // commit the renderer
  ospCommit(renderer);

  // clear frame buffer
  ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);
}

void GLFWOSPRayWindow::registerDisplayCallback(
    std::function<void(GLFWOSPRayWindow *)> callback)
{
  displayCallback = callback;
}

void GLFWOSPRayWindow::mainLoop()
{
  // continue until the user closes the window
  while (!glfwWindowShouldClose(glfwWindow)) {
    display();

    // poll and process events
    glfwPollEvents();
  }
}

void GLFWOSPRayWindow::reshape(const ospcommon::vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  // release the current frame buffer, if it exists
  if (framebuffer)
    ospRelease(framebuffer);

  // create new frame buffer
  framebuffer = ospNewFrameBuffer(*reinterpret_cast<osp::vec2i *>(&windowSize),
                                  OSP_FB_SRGBA,
                                  OSP_FB_COLOR | OSP_FB_ACCUM);

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));
  ospCommit(camera);
}

void GLFWOSPRayWindow::motion(const ospcommon::vec2f &position)
{
  static ospcommon::vec2f previousMouse(-1);

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
      ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

      ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));
      ospSetVec3f(camera,
                  "pos",
                  osp::vec3f{arcballCamera->eyePos().x,
                             arcballCamera->eyePos().y,
                             arcballCamera->eyePos().z});
      ospSetVec3f(camera,
                  "dir",
                  osp::vec3f{arcballCamera->lookDir().x,
                             arcballCamera->lookDir().y,
                             arcballCamera->lookDir().z});
      ospSetVec3f(camera,
                  "up",
                  osp::vec3f{arcballCamera->upDir().x,
                             arcballCamera->upDir().y,
                             arcballCamera->upDir().z});

      ospCommit(camera);
    }
  }

  previousMouse = mouse;
}

void GLFWOSPRayWindow::display()
{
  // clock used to compute frame rate
  static auto displayStart = std::chrono::high_resolution_clock::now();

  // if a display callback has been registered, call it
  if (displayCallback) {
    displayCallback(this);
  }

  // render OSPRay frame
  ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

  // map OSPRay frame buffer, update OpenGL texture with its contents, then
  // unmap
  uint32_t *fb = (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);

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

  ospUnmapFrameBuffer(fb, framebuffer);

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

  // swap buffers
  glfwSwapBuffers(glfwWindow);

  // display frame rate in window title
  auto displayEnd = std::chrono::high_resolution_clock::now();
  auto durationMilliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
                                                            displayStart);
  displayStart = displayEnd;

  const float frameRate = 1000.f / float(durationMilliseconds.count());

  std::stringstream windowTitle;
  windowTitle << "OSPRay: " << std::setprecision(3) << frameRate << " fps";

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}
