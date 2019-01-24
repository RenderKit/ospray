// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#include <random>
#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#if SHOW_WINDOW
#include <GLFW/glfw3.h>
#endif
#include <mpiCommon/MPICommon.h>
#include <mpi.h>
#include "ospcommon/utility/SaveImage.h"
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Geometry.h"
#include "ospray/ospray_cpp/Renderer.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "ospray/ospray_cpp/Volume.h"
#include "ospray/ospray_cpp/Model.h"
#include "widgets/transferFunction.h"
#include "ospcommon/containers/AlignedVector.h"
#include "common/sg/transferFunction/TransferFunction.h"
#include "widgets/imgui_impl_glfw_gl3.h"
#include "common/imgui/imgui.h"
#include "gensv/generateSciVis.h"
#include "arcball.h"
#include "common/sg/3rdParty/tiny_obj_loader.h"

/* This app demonstrates how to write an distributed scivis style
 * interactive renderer using the distributed MPI device. Note that because
 * OSPRay uses sort-last compositing it is up to the user to ensure
 * that the data distribution across the nodes is suitable. Specifically,
 * each nodes' data must be convex and disjoint. This renderer only
 * supports multiple volumes and geometries per-node, to ensure they're
 * composited correctly you specify a list of bounding regions to the
 * model, within these regions can be arbitrary volumes/geometrys
 * and each rank can have as many regions as needed. As long as the
 * regions are disjoint/convex the data will be rendered correctly.
 * In this example we set two regions on certain ranks just to produce
 * a gap in the ranks volume to demonstrate how they work.
 *
 * In the case that you have geometry crossing the boundary of nodes
 * and are replicating it on both nodes to render (ghost zones, etc.)
 * the region will be used by the renderer to clip rays against allowing
 * to split the object between the two nodes, with each rendering half.
 * This will keep the regions rendered by each rank disjoint and thus
 * avoid any artifacts. For example, if a sphere center is on the border
 * between two nodes, each would render half the sphere and the halves
 * would be composited to produce the final complete sphere in the image.
 *
 * See gensv::loadVolume for an example of how to properly load a volume
 * distributed across ranks with correct specification of brick positions
 * and ghost voxels. If no volume file data is passed a volume will be
 * generated instead, in that case see gensv::makeVolume.
 */

using namespace ospray::cpp;
using namespace ospcommon;

// Commandline params
std::string volumeFile = "";
std::string dtype = "";
vec3i dimensions = vec3i(-1);
vec2f valueRange = vec2f(-1);

size_t nSpheres = 0;
FileName transferFcnFile;
float sphereRadius = 1.0;
bool transparentSpheres = false;

int aoSamples = 0;
int shadowsEnabled = 0;

vec2i imgSize(512, 512);

// Struct for bcasting out the camera change info and general app state
struct AppState {
  // eye pos, look dir, up dir
  std::array<vec3f, 3> v;
  vec2i fbSize;
  bool cameraChanged, quit, fbSizeChanged, tfcnChanged;

  AppState() : fbSize(imgSize), cameraChanged(false), quit(false),
    fbSizeChanged(false)
  {}
};

// Extra stuff we need in GLFW callbacks
struct WindowState {
  Arcball &camera;
  vec2f prevMouse;
  bool cameraChanged;
  AppState &app;
  bool isImGuiHovered;

  WindowState(AppState &app, Arcball &camera)
    : camera(camera), prevMouse(-1), cameraChanged(false), app(app),
    isImGuiHovered(false)
  {}
};

#if SHOW_WINDOW
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;
      case GLFW_KEY_P: {
          const vec3f eye = state->camera.eyePos();
          const vec3f center = state->camera.center();
          const vec3f up = state->camera.upDir();
          std::cout << "-vp " << eye.x << " " << eye.y << " " << eye.z
            << "\n-vu " << up.x << " " << " " << up.y << " " << up.z
            << "\n-vi " << center.x << " " << center.y << " " << center.z
            << "\n";
        }
        break;
      default:
        break;
    }
  }
  // Forward on to ImGui
  ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}
void cursorPosCallback(GLFWwindow *window, double x, double y) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  if (state->isImGuiHovered) {
    return;
  }
  const vec2f mouse(x, y);
  if (state->prevMouse != vec2f(-1)) {
    const bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const vec2f prev = state->prevMouse;
    state->cameraChanged = leftDown || rightDown || middleDown;

    if (leftDown) {
      const vec2f mouseFrom(clamp(prev.x * 2.f / state->app.fbSize.x - 1.f,  -1.f, 1.f),
                            clamp(prev.y * 2.f / state->app.fbSize.y - 1.f,  -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / state->app.fbSize.x - 1.f,  -1.f, 1.f),
                          clamp(mouse.y * 2.f / state->app.fbSize.y - 1.f,  -1.f, 1.f));
      state->camera.rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      state->camera.zoom(mouse.y - prev.y);
    } else if (middleDown) {
      state->camera.pan(vec2f(prev.x - mouse.x, prev.y - mouse.y));
    }
  }
  state->prevMouse = mouse;
}
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  state->app.fbSize = vec2i(width, height);
  state->app.fbSizeChanged = true;
}
void charCallback(GLFWwindow *, unsigned int c) {
  ImGuiIO& io = ImGui::GetIO();
  if (c > 0 && c < 0x10000) {
    io.AddInputCharacter((unsigned short)c);
  }
}
#endif

void parseArgs(int argc, char **argv)
{
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-f") {
      volumeFile = argv[++i];
    } else if (arg == "-dtype") {
      dtype = argv[++i];
    } else if (arg == "-dims") {
      dimensions.x = std::atoi(argv[++i]);
      dimensions.y = std::atoi(argv[++i]);
      dimensions.z = std::atoi(argv[++i]);
    } else if (arg == "-range") {
      valueRange.x = std::atof(argv[++i]);
      valueRange.y = std::atof(argv[++i]);
    } else if (arg == "-spheres") {
      nSpheres = std::atol(argv[++i]);
    } else if (arg == "-tfn") {
      transferFcnFile = argv[++i];
    } else if (arg == "-radius") {
      sphereRadius = std::stof(argv[++i]);
    } else if (arg == "-transparent-spheres") {
      transparentSpheres = true;
    } else if (arg == "-ao") {
      aoSamples = std::stoi(argv[++i]);
    } else if (arg == "-w") {
      imgSize.x = std::atoi(argv[++i]);
    } else if (arg == "-h") {
      imgSize.y = std::atoi(argv[++i]);
    } else if (arg == "-shadows") {
      shadowsEnabled = 1;
    }
  }
  if (!volumeFile.empty()) {
    if (dtype.empty()) {
      std::cerr << "Error: -dtype (uchar|char|float|double) is required\n";
      std::exit(1);
    }
    if (dimensions == vec3i(-1)) {
      std::cerr << "Error: -dims X Y Z is required to pass volume dims\n";
      std::exit(1);
    }
    if (valueRange == vec2f(-1)) {
      std::cerr << "Error: -range X Y is required to set transfer function range\n";
      std::exit(1);
    }
  }
}

void runApp()
{
  ospLoadModule("mpi");
  Device device("mpi_distributed");
  device.set("masterRank", 0);
  ospDeviceSetStatusFunc(device.handle(),
                         [](const char *msg) {
                           std::cout << "OSP Status: " << msg << "\n";
                         });
  ospDeviceSetErrorFunc(device.handle(),
                        [](OSPError err, const char *msg) {
                          std::cout << "OSP Error: " <<  msg << "\n";
                        });
  device.commit();
  device.setCurrent();

  const int rank = mpicommon::world.rank;
  const int worldSize = mpicommon::world.size;
  std::cout << "Rank " << rank << "/" << worldSize << "\n";

  AppState app;
  gensv::LoadedVolume volume;
  box3f worldBounds;

  if (!volumeFile.empty()) {
    volume = gensv::loadVolume(volumeFile,
                                dimensions,
                                dtype,
                                valueRange);
    worldBounds = box3f(vec3f(0), vec3f(dimensions));
  } else {
    volume = gensv::makeVolume();

    const vec3f upperBound = vec3f(128) * gensv::computeGrid(worldSize);
    worldBounds = box3f(vec3f(0), upperBound);
  }

  Model model;
  Model ghostModel;
  volume.volume.commit();
  model.addVolume(volume.volume);

  // All ranks generate the same sphere data to mimic rendering a distributed
  // shared dataset
  if (nSpheres != 0) {
    auto spheres = gensv::makeSpheres(worldBounds, nSpheres,
        sphereRadius, transparentSpheres);
    model.addGeometry(spheres);

    ghostModel.addGeometry(spheres);
    ghostModel.set("id", rank);
    ghostModel.commit();
  }

  // Clip off any ghost voxels or spheres
  model.set("region.lower", volume.bounds.lower);
  model.set("region.upper", volume.bounds.upper);
  model.set("id", rank);
  model.commit();

  Arcball arcballCamera(worldBounds, imgSize);

  Camera camera("perspective");
  camera.set("pos", arcballCamera.eyePos());
  camera.set("dir", arcballCamera.lookDir());
  camera.set("up", arcballCamera.upDir());
  camera.set("aspect", static_cast<float>(app.fbSize.x) / app.fbSize.y);
  camera.commit();

  Light ambientLight("ambient");
  ambientLight.set("intensity", 0.2);
  ambientLight.commit();
  Light distantLight("distant");
  distantLight.set("direction", vec3f(-1, -1, 1));
  distantLight.commit();

  std::array<OSPObject, 2> lights = {distantLight.object(), ambientLight.object()};
  Data lightData(2, OSP_OBJECT, lights.data());
  lightData.commit();

  Renderer renderer("mpi_raycast");
  renderer.set("model", model);
  if (nSpheres != 0) {
    renderer.set("ghostModel", ghostModel);
  }
  renderer.set("camera", camera);
  renderer.set("aoSamples", aoSamples);
  renderer.set("shadowsEnabled", shadowsEnabled);
  renderer.set("bgColor", vec3f(0.01f));
  renderer.set("lights", lightData);
  renderer.commit();
  assert(renderer);

  const int fbFlags = OSP_FB_COLOR | OSP_FB_ACCUM;

  OSPFrameBufferFormat fbColorFormat = OSP_FB_SRGBA;
  FrameBuffer fb(app.fbSize, fbColorFormat, fbFlags);
  fb.commit();
  fb.clear(fbFlags);

  mpicommon::world.barrier();

  std::mt19937 rng;
  std::uniform_real_distribution<float> randomCamDistrib;

  containers::AlignedVector<vec3f> tfcnColors;
  containers::AlignedVector<float> tfcnAlphas;

  std::shared_ptr<ospray::sg::TransferFunction> transferFcn = nullptr;
  std::shared_ptr<ospray::imgui3D::TransferFunction> tfnWidget = nullptr;
  std::shared_ptr<WindowState> windowState;
  GLFWwindow *window = nullptr;
  if (rank == 0) {
    transferFcn = std::make_shared<ospray::sg::TransferFunction>();

#if SHOW_WINDOW
    tfnWidget = std::make_shared<ospray::imgui3D::TransferFunction>(transferFcn);
    tfnWidget->loadColorMapPresets();
#endif

    if (!transferFcnFile.str().empty()) {
#if SHOW_WINDOW
      tfnWidget->load(transferFcnFile);
#else
      tfn::TransferFunction loaded(transferFcnFile);
      transferFcn->child("valueRange").setValue(valueRange);

      auto &colorCP = *transferFcn->child("colorControlPoints").nodeAs<ospray::sg::DataVector4f>();
      auto &opacityCP = *transferFcn->child("opacityControlPoints").nodeAs<ospray::sg::DataVector2f>();
      opacityCP.clear();
      colorCP.clear();

      for (size_t i = 0; i < loaded.rgbValues.size(); ++i) {
        colorCP.push_back(vec4f(static_cast<float>(i) / loaded.rgbValues.size(),
                                loaded.rgbValues[i].x,
                                loaded.rgbValues[i].y,
                                loaded.rgbValues[i].z));
      }
      for (size_t i = 0; i < loaded.opacityValues.size(); ++i) {
        const float x = (loaded.opacityValues[i].x - loaded.dataValueMin)
                      / (loaded.dataValueMax - loaded.dataValueMin);
        opacityCP.push_back(vec2f(x, loaded.opacityValues[i].y));
      }

      opacityCP.markAsModified();
      colorCP.markAsModified();

      transferFcn->updateChildDataValues();
      auto &colors    = *transferFcn->child("colors").nodeAs<ospray::sg::DataBuffer>();
      auto &opacities = *transferFcn->child("opacities").nodeAs<ospray::sg::DataBuffer>();

      colors.markAsModified();
      opacities.markAsModified();

      transferFcn->commit();

      tfcnColors = transferFcn->child("colors").nodeAs<ospray::sg::DataVector3f>()->v;
      const auto &ospAlpha = transferFcn->child("opacities").nodeAs<ospray::sg::DataVector1f>()->v;
      tfcnAlphas.clear();
      std::copy(ospAlpha.begin(), ospAlpha.end(), std::back_inserter(tfcnAlphas));
      app.tfcnChanged = true;
#endif
    }

#if SHOW_WINDOW
    if (!glfwInit()) {
      std::cerr << "Failed to init GLFW" << std::endl;
      std::exit(1);
    }
    window = glfwCreateWindow(app.fbSize.x, app.fbSize.y,
        "Sample Distributed OSPRay Viewer", nullptr, nullptr);
    if (!window) {
      glfwTerminate();
      std::cerr << "Failed to create window" << std::endl;
      std::exit(1);
    }
    glfwMakeContextCurrent(window);

    windowState = std::make_shared<WindowState>(app, arcballCamera);

    ImGui_ImplGlfwGL3_Init(window, false);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowUserPointer(window, windowState.get());
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
    glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
    glfwSetCharCallback(window, charCallback);
#endif
  }

  while (!app.quit) {
    using namespace std::chrono;

    if (app.cameraChanged) {
      camera.set("pos", app.v[0]);
      camera.set("dir", app.v[1]);
      camera.set("up", app.v[2]);
      camera.commit();

      fb.clear(fbFlags);
      app.cameraChanged = false;
    }
    auto startFrame = high_resolution_clock::now();
    renderer.renderFrame(fb, OSP_FB_COLOR);
    auto endFrame = high_resolution_clock::now();

    const int renderTime = duration_cast<milliseconds>(endFrame - startFrame).count();

    if (rank == 0) {
      glClear(GL_COLOR_BUFFER_BIT);
      uint32_t *img = (uint32_t*)fb.map(OSP_FB_COLOR);
      glDrawPixels(app.fbSize.x, app.fbSize.y, GL_RGBA, GL_UNSIGNED_BYTE, img);
      fb.unmap(img);

#if SHOW_WINDOW
      const auto tfcnTimeStamp = transferFcn->childrenLastModified();

      ImGui_ImplGlfwGL3_NewFrame();
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
          1000.0f / ImGui::GetIO().Framerate,
          ImGui::GetIO().Framerate);
      ImGui::Text("OSPRay render time %d ms/frame", renderTime);
      tfnWidget->drawUi();
      ImGui::Render();

      glfwSwapBuffers(window);

      glfwPollEvents();
      if (glfwWindowShouldClose(window)) {
        app.quit = true;
      }

      tfnWidget->render();

      if (transferFcn->childrenLastModified() != tfcnTimeStamp) {
        transferFcn->child("valueRange").setValue(valueRange);
        transferFcn->updateChildDataValues();
        tfcnColors = transferFcn->child("colors").nodeAs<ospray::sg::DataVector3f>()->v;
        const auto &ospAlpha = transferFcn->child("opacities").nodeAs<ospray::sg::DataVector1f>()->v;
        tfcnAlphas.clear();
        std::copy(ospAlpha.begin(), ospAlpha.end(), std::back_inserter(tfcnAlphas));
        app.tfcnChanged = true;
      }

      const vec3f eye = windowState->camera.eyePos();
      const vec3f look = windowState->camera.lookDir();
      const vec3f up = windowState->camera.upDir();
      app.v[0] = vec3f(eye.x, eye.y, eye.z);
      app.v[1] = vec3f(look.x, look.y, look.z);
      app.v[2] = vec3f(up.x, up.y, up.z);
      app.cameraChanged = windowState->cameraChanged;
      windowState->cameraChanged = false;
      windowState->isImGuiHovered = ImGui::IsMouseHoveringAnyWindow();
#endif
    }
    // Send out the shared app state that the workers need to know, e.g. camera
    // position, if we should be quitting.
    MPI_Bcast(&app, sizeof(AppState), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (app.fbSizeChanged) {
      fb = FrameBuffer(app.fbSize, OSP_FB_SRGBA, fbFlags);
      fb.clear(fbFlags);
      camera.set("aspect", static_cast<float>(app.fbSize.x) / app.fbSize.y);
      camera.commit();

      arcballCamera.updateScreen(vec2i(app.fbSize.x, app.fbSize.y));
      app.fbSizeChanged = false;
      if (rank == 0) {
        glViewport(0, 0, app.fbSize.x, app.fbSize.y);
      }
    }
    if (app.tfcnChanged) {
      size_t sz = tfcnColors.size();
      MPI_Bcast(&sz, sizeof(size_t), MPI_BYTE, 0, MPI_COMM_WORLD);
      if (rank != 0) {
        tfcnColors.resize(sz);
      }
      MPI_Bcast(tfcnColors.data(), sizeof(vec3f) * tfcnColors.size(), MPI_BYTE,
                0, MPI_COMM_WORLD);

      sz = tfcnAlphas.size();
      MPI_Bcast(&sz, sizeof(size_t), MPI_BYTE, 0, MPI_COMM_WORLD);
      if (rank != 0) {
        tfcnAlphas.resize(sz);
      }
      MPI_Bcast(tfcnAlphas.data(), sizeof(float) * tfcnAlphas.size(), MPI_BYTE,
                0, MPI_COMM_WORLD);

      Data colorData(tfcnColors.size(), OSP_FLOAT3, tfcnColors.data());
      Data alphaData(tfcnAlphas.size(), OSP_FLOAT, tfcnAlphas.data());
      colorData.commit();
      alphaData.commit();

      volume.tfcn.set("colors", colorData);
      volume.tfcn.set("opacities", alphaData);
      volume.tfcn.commit();

      fb.clear(fbFlags);
      app.tfcnChanged = false;
    }

#if !SHOW_WINDOW
    const vec3f eye = arcballCamera.eyePos();
    const vec3f look = arcballCamera.lookDir();
    const vec3f up = arcballCamera.upDir();
    app.v[0] = vec3f(eye.x, eye.y, eye.z);
    app.v[1] = vec3f(look.x, look.y, look.z);
    app.v[2] = vec3f(up.x, up.y, up.z);
#endif
  }
  if (rank == 0) {
      ImGui_ImplGlfwGL3_Shutdown();
      glfwDestroyWindow(window);
  }

}

int main(int argc, char **argv) {
  parseArgs(argc, argv);

  // The application can be responsible for initializing and finalizing MPI,
  // or can let OSPRay's mpi_distributed device handle it. In the case that
  // the distributed device is responsible MPI will be initialized when the
  // device is created and finalized when it's destroyed.
  int provided = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  assert(provided == MPI_THREAD_MULTIPLE);

  runApp();

  ospShutdown();

  // If the app is responsible for setting up MPI we've also got
  // to finalize it at the exit
  MPI_Finalize();

  return 0;
}

