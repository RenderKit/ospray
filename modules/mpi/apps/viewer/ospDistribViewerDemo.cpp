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
#include <GLFW/glfw3.h>
#include <mpiCommon/MPICommon.h>
#include <mpi.h>
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
#include "gensv/llnlrm_reader.h"

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
float varianceThreshold = 0.0f;
FileName transferFcnFile;
bool appInitMPI = false;
size_t nlocalBricks = 1;
float sphereRadius = 0.005;
bool transparentSpheres = false;
int aoSamples = 0;
int nBricks = -1;
int bricksPerRank = 1;
bool llnlrm = false;
bool fbnone = false;
const int IMG_SIZE = 512;

std::vector<std::string> osp_bricks;

struct RIVLFile {
  std::string file;
  size_t ofsVerts, numVerts;
  size_t ofsPrims, numPrims;
};

RIVLFile rivl;

// Struct for bcasting out the camera change info and general app state
struct AppState {
  // eye pos, look dir, up dir
  std::array<vec3f, 3> v;
  vec2i fbSize;
  bool cameraChanged, quit, fbSizeChanged, tfcnChanged;

  AppState() : fbSize(IMG_SIZE), cameraChanged(false), quit(false),
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
      state->camera.pan(vec2f(mouse.x - prev.x, prev.y - mouse.y));
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
    } else if (arg == "-variance") {
      varianceThreshold = std::atof(argv[++i]);
    } else if (arg == "-tfn") {
      transferFcnFile = argv[++i];
    } else if (arg == "-appMPI") {
      appInitMPI = true;
    } else if (arg == "-nlocal-bricks") {
      nlocalBricks = std::stol(argv[++i]);
    } else if (arg == "-radius") {
      sphereRadius = std::stof(argv[++i]);
    } else if (arg == "-transparent-spheres") {
      transparentSpheres = true;
    } else if (arg == "-ao") {
      aoSamples = std::stoi(argv[++i]);
    } else if (arg == "-nbricks") {
      nBricks = std::stoi(argv[++i]);
    } else if (arg == "-bpr") {
      bricksPerRank = std::stoi(argv[++i]);
    } else if (arg == "-bob") {
      llnlrm = true;
    } else if (arg == "-rivl") {
      rivl.file = argv[++i];
      rivl.ofsVerts = std::stoull(argv[++i]);
      rivl.numVerts = std::stoull(argv[++i]);
      rivl.ofsPrims = std::stoull(argv[++i]);
      rivl.numPrims = std::stoull(argv[++i]);
    } else if (arg == "-osp-brick") {
      ++i;
      for (; i < argc && argv[i][0] != '-'; ++i) {
        osp_bricks.push_back(argv[i]);
      }
    } else if (arg == "-fb-none") {
      fbnone = true;
    }
  }
  if (!volumeFile.empty() && !llnlrm) {
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
    if (nlocalBricks != 1) {
      std::cerr << "Error: -nlocal-bricks only makes supported for generated volumes\n";
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
  containers::AlignedVector<gensv::LoadedVolume> volumes;
  containers::AlignedVector<gensv::SharedVolumeBrick> bricks;
  box3f worldBounds;
  if (!osp_bricks.empty()) {
    if (osp_bricks.size() != worldSize) {
      throw std::runtime_error("OSP Brick count must match number of ranks");
    }
    const std::string my_brick = osp_bricks[rank];
    bricks.push_back(gensv::loadOSPBrick(my_brick, valueRange));

    MPI_Allreduce(&bricks[0].region.bounds.lower, &worldBounds.lower, 3,
                  MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&bricks[0].region.bounds.upper, &worldBounds.upper, 3,
                  MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);

    std::cout << "World bounds: " << worldBounds << "\n";

  } else if (!volumeFile.empty()) {
    if (nBricks == -1) {
      nBricks = worldSize;
    }
    if (!llnlrm) {
      bricks = gensv::loadBrickedVolume(volumeFile, dimensions,
                                        dtype, valueRange,
                                        nBricks, bricksPerRank);
    } else {
      bricks = gensv::loadRMBricks(volumeFile, bricksPerRank);
      dimensions = gensv::LLNLRMReader::dimensions();
    }

    worldBounds = box3f(vec3f(0), vec3f(dimensions));

    // Pick a nice sphere radius for a consisten voxel size to
    // sphere size ratio
    sphereRadius *= dimensions.x;
  } else {
    volumes = gensv::makeVolumes(rank * nlocalBricks, nlocalBricks,
        worldSize * nlocalBricks);

    // Translate the volume to center it
    worldBounds = box3f(vec3f(-0.5), vec3f(0.5));
    for (auto &v : volumes) {
      v.bounds.lower -= vec3f(0.5f);
      v.bounds.upper -= vec3f(0.5f);
      v.volume.set("gridOrigin", v.ghostGridOrigin - vec3f(0.5f));
    }
  }

  containers::AlignedVector<Model> models, ghostModels;
  // Generated volumes
  for (size_t i = 0; i < volumes.size(); ++i) {
    auto &v = volumes[i];
    v.volume.commit();
    Model m;
    m.addVolume(v.volume);
    // All ranks generate the same sphere data to mimic rendering a distributed
    // shared dataset
    if (nSpheres != 0) {
      auto spheres = gensv::makeSpheres(worldBounds, nSpheres,
                                        sphereRadius, transparentSpheres);
      m.addGeometry(spheres);
      // If we're setting spheres we need to enforce an explicit region bound
      m.set("region.lower", v.bounds.lower);
      m.set("region.upper", v.bounds.upper);

      Model g;
      g.addGeometry(spheres);
      ghostModels.push_back(g);
    }
    m.set("id", int(rank * nlocalBricks + i));
    models.push_back(m);
  }

  // Loaded bricks
  for (size_t i = 0; i < bricks.size(); ++i) {
    auto &v = bricks[i].vol;
    v.volume.commit();
    Model m;
    m.addVolume(v.volume);
    // All ranks generate the same sphere data to mimic rendering a distributed
    // shared dataset
    if (nSpheres != 0) {
      auto spheres = gensv::makeSpheres(worldBounds, nSpheres,
                                        sphereRadius, transparentSpheres);
      m.addGeometry(spheres);
      // If we're setting spheres we need to enforce an explicit region bound
      m.set("region.lower", v.bounds.lower);
      m.set("region.upper", v.bounds.upper);

      Model g;
      g.addGeometry(spheres);
      ghostModels.push_back(g);
    }
    m.set("id", bricks[i].region.id);
    models.push_back(m);
  }

  /*
  if (!rivl.file.empty()) {
    std::cout << "loading rivl " << rivl.file << "\n";
    FILE *fp = fopen(rivl.file.c_str(), "rb");
    std::vector<vec3f> verts(rivl.numVerts, vec3f(0));
    std::vector<vec4i> prims(rivl.numPrims, vec4i(0));
    if (fread(verts.data(), sizeof(vec3f), verts.size(), fp) != rivl.numVerts) {
      throw std::runtime_error("error reading rivl verts");
    }
    if (fread(prims.data(), sizeof(vec4i), prims.size(), fp) != rivl.numPrims) {
      throw std::runtime_error("error reading rivl prims");
      
    }
    Data vertData(verts.size(), OSP_FLOAT3, verts.data());
    Data primData(prims.size(), OSP_INT4, prims.data());
    vertData.commit();
    primData.commit();
    Geometry mesh("triangles");
    mesh.set("vertex", vertData);
    mesh.set("index", primData);
    mesh.commit();
    model.addGeometry(mesh);
  }
  */
  for (auto &m : models) {
    m.commit();
  }
  for (auto &m : ghostModels) {
    m.commit();
  }

  Arcball arcballCamera(worldBounds, vec2i(IMG_SIZE, IMG_SIZE));

  Camera camera("perspective");
  camera.set("pos", arcballCamera.eyePos());
  camera.set("dir", arcballCamera.lookDir());
  camera.set("up", arcballCamera.upDir());
  camera.set("aspect", static_cast<float>(app.fbSize.x) / app.fbSize.y);
  camera.commit();

  Renderer renderer("mpi_raycast");

  std::vector<OSPModel> modelHandles;
  std::transform(models.begin(), models.end(), std::back_inserter(modelHandles),
                 [](const Model &m) { return m.handle(); });
  Data modelsData(modelHandles.size(), OSP_OBJECT, modelHandles.data());

  std::vector<OSPModel> ghostModelHandles;
  std::transform(ghostModels.begin(), ghostModels.end(), std::back_inserter(ghostModelHandles),
                 [](const Model &m) { return m.handle(); });
  Data ghostModelsData(ghostModelHandles.size(), OSP_OBJECT, ghostModelHandles.data());

  renderer.set("model", modelsData);
  renderer.set("ghostModel", ghostModelsData);
  renderer.set("camera", camera);
  renderer.set("bgColor", vec4f(0.02, 0.02, 0.02, 0.0));
  renderer.set("varianceThreshold", varianceThreshold);
  renderer.set("aoSamples", aoSamples);
  renderer.commit();
  assert(renderer);

  const int fbFlags = OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE;
  OSPFrameBufferFormat fbColorFormat = OSP_FB_SRGBA;
  if (fbnone) {
    fbColorFormat = OSP_FB_NONE;
  }
  FrameBuffer fb(app.fbSize, fbColorFormat, fbFlags);
  if (fbnone) {
    PixelOp pixelOp("debug");
    pixelOp.set("prefix", "distrib-viewer");
    pixelOp.commit();
    fb.setPixelOp(pixelOp);
  }
  fb.commit();
  fb.clear(fbFlags);

  mpicommon::world.barrier();

  std::shared_ptr<ospray::sg::TransferFunction> transferFcn;
  std::shared_ptr<ospray::imgui3D::TransferFunction> tfnWidget;
  std::shared_ptr<WindowState> windowState;
  GLFWwindow *window = nullptr;
  if (rank == 0) {
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
    transferFcn = std::make_shared<ospray::sg::TransferFunction>();
    tfnWidget = std::make_shared<ospray::imgui3D::TransferFunction>(transferFcn);
    tfnWidget->loadColorMapPresets();
    if (!transferFcnFile.str().empty()) {
      tfnWidget->load(transferFcnFile);
    }

    ImGui_ImplGlfwGL3_Init(window, false);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowUserPointer(window, windowState.get());
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
    glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
    glfwSetCharCallback(window, charCallback);
  }

  containers::AlignedVector<vec3f> tfcnColors;
  containers::AlignedVector<float> tfcnAlphas;
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
      if (!fbnone) {
        uint32_t *img = (uint32_t*)fb.map(OSP_FB_COLOR);
        glDrawPixels(app.fbSize.x, app.fbSize.y, GL_RGBA, GL_UNSIGNED_BYTE, img);
        fb.unmap(img);
      }

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

      for (auto &v : volumes) {
        v.tfcn.set("colors", colorData);
        v.tfcn.set("opacities", alphaData);
        v.tfcn.commit();
      }
      for (auto &b : bricks) {
        b.vol.tfcn.set("colors", colorData);
        b.vol.tfcn.set("opacities", alphaData);
        b.vol.tfcn.commit();
      }

      fb.clear(fbFlags);
      app.tfcnChanged = false;
    }
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
  if (appInitMPI) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    assert(provided == MPI_THREAD_MULTIPLE);
  }

  runApp();

  ospShutdown();
  // If the app is responsible for setting up MPI we've also got
  // to finalize it at the exit
  if (appInitMPI) {
    MPI_Finalize();
  }
  return 0;
}

