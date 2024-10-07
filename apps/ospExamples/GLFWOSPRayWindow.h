// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
// glfw
#include "GLFW/glfw3.h"
// ospray
#include "ospray/ospray_cpp.h"
#include "rkcommon/containers/TransactionalBuffer.h"
// std
#include <functional>

using namespace rkcommon::math;
using namespace ospray;

// matches index in g_renderers
enum class OSPRayRendererType
{
  SCIVIS = 0,
  PATHTRACER = 1,
  AO = 2,
  DEBUGGER = 3,
  OTHER
};

class GLFWOSPRayWindow
{
 public:
  GLFWOSPRayWindow(const vec2i &windowSize,
      bool denoiserAvail = false,
      bool denoiserGPUAvail = false);

  ~GLFWOSPRayWindow();

  static GLFWOSPRayWindow *getActiveWindow();

  void mainLoop();

 protected:
  void addObjectToCommit(OSPObject obj);

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
  void refreshFrameOperations();
  void updateTargetFrames(const bool commit = true);

  static GLFWOSPRayWindow *activeWindow;

  vec2i windowSize;
  vec2f previousMouse{-1.f};

  bool denoiserAvailable{false};
  bool updateFrameOpsNextFrame{false};
  bool denoiserEnabled{false};
  bool renderSunSky{false};
  bool cancelFrameOnInteraction{false};
  bool showUnstructuredCells{false};

  OSPFrameBufferChannel showFBChannel{OSP_FB_COLOR};
  bool showFBAlpha{false};

  // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;
  affine3f lastXfm{one};
  OSPStereoMode cameraStereoMode{OSP_STEREO_NONE};
  float cameraMotionBlur{0.0f};
  float cameraRollingShutter{0.0f};
  OSPShutterType cameraShutterType{OSP_SHUTTER_GLOBAL};
  // only one frame during movement is rendered with MB,
  // during accumulation the camera is static and thus no MB
  bool renderCameraMotionBlur{false};

  // OSPRay objects managed by this class
  cpp::Renderer rendererPT{"pathtracer"};
  cpp::Renderer rendererSV{"scivis"};
  cpp::Renderer rendererAO{"ao"};
  cpp::Renderer rendererDBG{"debug"};
  cpp::Renderer *renderer{nullptr};
  cpp::Camera camera{"perspective"};
  cpp::World world;
  cpp::Light sunSky{"sunSky"};
  cpp::FrameBuffer framebuffer;
  cpp::Future currentFrame;
  cpp::Texture backplateTex{"texture2d"};
  cpp::ImageOperation denoiser;

  int frame{0};
  bool blueNoise{true};
  int accumLimit{0};

  vec3f bgColor{0.f};
  vec3f sunDirection{-0.25f, -1.0f, 1.0f};
  float turbidity{3.f};
  float horizonExtension{0.25f};

  std::string scene{"cornell_box"};

  std::string curveVariant{"bspline"};

  OSPRayRendererType rendererType{OSPRayRendererType::PATHTRACER};
  std::string rendererTypeStr{"pathtracer"};

  std::string pixelFilterTypeStr{"gaussian"};

  // List of OSPRay handles to commit before the next frame
  rkcommon::containers::TransactionalBuffer<OSPObject> objectsToCommit;

  // OpenGL framebuffer texture
  GLuint framebufferTexture = 0;

  // optional registered display callback, called before every display()
  std::function<void(GLFWOSPRayWindow *)> displayCallback;

  // toggles display of ImGui UI, if an ImGui callback is provided
  bool showUi = true;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;

  // FPS measurement of last frame
  uint32_t framesCounter{0};
  float latestFPS{0.f};
  float displayFPS{0.f};
};
