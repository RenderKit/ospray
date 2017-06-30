#include <random>
#include <array>
#include <chrono>
#include <GLFW/glfw3.h>
#include <mpiCommon/MPICommon.h>
#include <mpi.h>
#include <ospcommon/AffineSpace.h>
#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Data.h>
#include <ospray/ospray_cpp/Device.h>
#include <ospray/ospray_cpp/FrameBuffer.h>
#include <ospray/ospray_cpp/Geometry.h>
#include <ospray/ospray_cpp/Renderer.h>
#include <ospray/ospray_cpp/TransferFunction.h>
#include <ospray/ospray_cpp/Volume.h>
#include <ospray/ospray_cpp/Model.h>
#include "gensv/generateSciVis.h"

/* This app demonstrates how to write an distributed scivis style
 * interactive renderer using the distributed MPI device. Note that because
 * OSPRay uses sort-last compositing it is up to the user to ensure
 * that the data distribution across the nodes is suitable. Specifically,
 * each nodes' data must be convex and disjoint. This renderer only
 * supports multiple volumes and geometries per-node, to ensure they're
 * composited correctly you specify a list of bounding regions to the
 * model, within these regions can be arbitrary volumes/geometries
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

struct Sphere {
  vec3f org;
  int colorID{0};
};

struct Arcball {
  Arcball(const box3f &worldBounds);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const vec2f &from, const vec2f &to);
  void zoom(float amount);
  vec3f eyePos() const;
  vec3f lookDir() const;
  vec3f upDir() const;

private:
  void updateCamera();
  // Project the point in [-1, 1] screen space onto the arcball sphere
  Quaternion3f screenToArcball(const vec2f &p);

  float zoomSpeed;
  AffineSpace3f lookAt, translation, inv_camera;
  Quaternion3f rotation;
};

Arcball::Arcball(const box3f &worldBounds)
  : translation(one), rotation(one)
{
  vec3f diag = worldBounds.size();
  zoomSpeed = length(diag) / 150.0;
  diag = max(diag, vec3f(0.3f * length(diag)));

  lookAt = AffineSpace3f::lookat(vec3f(0, 0, 1), vec3f(0, 0, 0), vec3f(0, 1, 0));
  translation = AffineSpace3f::translate(vec3f(0, 0, diag.z));
  updateCamera();
}
void Arcball::rotate(const vec2f &from, const vec2f &to) {
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}
void Arcball::zoom(float amount) {
  amount *= zoomSpeed;
  translation = AffineSpace3f::translate(vec3f(0, 0, amount)) * translation;
  updateCamera();
}
vec3f Arcball::eyePos() const {
  return xfmPoint(inv_camera, vec3f(0, 0, 1));
}
vec3f Arcball::lookDir() const {
  return xfmVector(inv_camera, vec3f(0, 0, 1));
}
vec3f Arcball::upDir() const {
  return xfmVector(inv_camera, vec3f(0, 1, 0));
}
void Arcball::updateCamera() {
  const AffineSpace3f rot = LinearSpace3f(rotation);
  const AffineSpace3f camera = translation * lookAt * rot;
  inv_camera = rcp(camera);
}
Quaternion3f Arcball::screenToArcball(const vec2f &p) {
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f){
    return Quaternion3f(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const vec2f unitDir = normalize(p);
    return Quaternion3f(0, unitDir.x, unitDir.y, 0);
  }
}

// Struct for bcasting out the camera change info and general app state
struct AppState {
  // eye pos, look dir, up dir
  std::array<vec3f, 3> v;
  vec2i fbSize;
  bool cameraChanged, quit, fbSizeChanged;

  AppState() : fbSize(1024), cameraChanged(false), quit(false), fbSizeChanged(false)
  {}
};
// Extra junk we need in GLFW callbacks
struct WindowState {
  Arcball &camera;
  vec2f prevMouse;
  bool cameraChanged;
  AppState &app;

  WindowState(AppState &app, Arcball &camera)
    : camera(camera), prevMouse(-1), cameraChanged(false), app(app)
  {}
};

void keyCallback(GLFWwindow *window, int key, int, int action, int) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;
      default:
        break;
    }
  }
}
void cursorPosCallback(GLFWwindow *window, double x, double y) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  const vec2f mouse(x, y);
  if (state->prevMouse != vec2f(-1)) {
    const bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const vec2f prev = state->prevMouse;

    if (leftDown) {
      const vec2f mouseFrom(clamp(prev.x * 2.f / state->app.fbSize.x - 1.f,  -1.f, 1.f),
                            clamp(1.f - 2.f * prev.y / state->app.fbSize.y, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / state->app.fbSize.x - 1.f,  -1.f, 1.f),
                          clamp(1.f - 2.f * mouse.y / state->app.fbSize.y, -1.f, 1.f));
      state->camera.rotate(mouseFrom, mouseTo);
      state->cameraChanged = true;
    } else if (rightDown) {
      state->camera.zoom(mouse.y - prev.y);
      state->cameraChanged = true;
    }
  }
  state->prevMouse = mouse;
}
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  state->app.fbSize = vec2i(width, height);
  state->app.fbSizeChanged = true;
}

int main(int argc, char **argv) {
  std::string volumeFile, dtype;
  vec3i dimensions = vec3i(-1);
  vec2f valueRange = vec2f(-1);
  size_t nSpheres = 100;
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
    }
  }
  if (!volumeFile.empty()) {
    if (dtype.empty()) {
      std::cerr << "Error: -dtype (uchar|char|float|double) is required\n";
      return 1;
    }
    if (dimensions == vec3i(-1)) {
      std::cerr << "Error: -dims X Y Z is required to pass volume dims\n";
      return 1;
    }
    if (valueRange == vec2f(-1)) {
      std::cerr << "Error: -range X Y is required to set transfer function range\n";
      return 1;
    }
  }

  ospLoadModule("mpi");
  int provided = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  assert(provided == MPI_THREAD_MULTIPLE);
  Device device("mpi_distributed");
  device.set("masterRank", 0);
  device.commit();
  device.setCurrent();

  const int rank = mpicommon::world.rank;
  const int worldSize = mpicommon::world.size;


  AppState app;
  Model model;
  std::pair<Volume, box3f> volume(nullptr, box3f());
  box3f worldBounds;
  float sphereRadius = 0.005;
  if (!volumeFile.empty()) {
    volume = gensv::loadVolume(volumeFile, dimensions, dtype, valueRange);

    // Translate the volume to center it
    const vec3f upper = vec3f(dimensions);
    const vec3i halfLength = dimensions / 2;
    worldBounds = box3f(vec3f(-halfLength), vec3f(halfLength));
    volume.second.lower -= vec3f(halfLength);
    volume.second.upper -= vec3f(halfLength);
    volume.first.set("gridOrigin", volume.second.lower);

    // Pick a nice sphere radius for a consisten voxel size to
    // sphere size ratio
    sphereRadius *= dimensions.x;
  } else {
    volume = gensv::makeVolume();
    // Translate the volume to center it
    worldBounds = box3f(vec3f(-0.5), vec3f(0.5));
    volume.second.lower -= vec3f(0.5f);
    volume.second.upper -= vec3f(0.5f);
    volume.first.set("gridOrigin", volume.second.lower);
  }
  volume.first.commit();
  model.addVolume(volume.first);
  if (nSpheres != 0) {
    auto spheres = gensv::makeSpheres(volume.second, nSpheres, sphereRadius);
    model.addGeometry(spheres);
  }

  Arcball arcballCamera(worldBounds);

  std::vector<box3f> regions{volume.second};
  ospray::cpp::Data regionData(regions.size() * 2, OSP_FLOAT3,
      regions.data());
  model.set("regions", regionData);

  model.commit();

  Camera camera("perspective");
  camera.set("pos", arcballCamera.eyePos());
  camera.set("dir", arcballCamera.lookDir());
  camera.set("up", arcballCamera.upDir());
  camera.set("aspect", static_cast<float>(app.fbSize.x) / app.fbSize.y);
  camera.commit();

  Renderer renderer("mpi_raycast");
  // Should just do 1 set here, which is read?
  renderer.set("world", model);
  renderer.set("model", model);
  renderer.set("camera", camera);
  renderer.set("bgColor", vec3f(0.02));
  renderer.commit();
  assert(renderer);

  FrameBuffer fb(app.fbSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
  fb.clear(OSP_FB_COLOR | OSP_FB_ACCUM);

  mpicommon::world.barrier();

  std::shared_ptr<WindowState> windowState;
  GLFWwindow *window = nullptr;
  if (rank == 0) {
    if (!glfwInit()) {
      return 1;
    }
    window = glfwCreateWindow(app.fbSize.x, app.fbSize.y,
        "Sample Distributed OSPRay Viewer", nullptr, nullptr);
    if (!window) {
      glfwTerminate();
      return 1;
    }
    windowState = std::make_shared<WindowState>(app, arcballCamera);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowUserPointer(window, windowState.get());
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwMakeContextCurrent(window);
  }

  while (!app.quit) {
    if (app.cameraChanged) {
      camera.set("pos", app.v[0]);
      camera.set("dir", app.v[1]);
      camera.set("up", app.v[2]);
      camera.commit();

      fb.clear(OSP_FB_COLOR | OSP_FB_ACCUM);
      app.cameraChanged = false;
    }
    renderer.renderFrame(fb, OSP_FB_COLOR);

    if (rank == 0) {
      glClear(GL_COLOR_BUFFER_BIT);
      uint32_t *img = (uint32_t*)fb.map(OSP_FB_COLOR);
      glDrawPixels(app.fbSize.x, app.fbSize.y, GL_RGBA, GL_UNSIGNED_BYTE, img);
      fb.unmap(img);
      glfwSwapBuffers(window);

      glfwPollEvents();
      if (glfwWindowShouldClose(window)) {
        app.quit = true;
      }

      const vec3f eye = windowState->camera.eyePos();
      const vec3f look = windowState->camera.lookDir();
      const vec3f up = windowState->camera.upDir();
      app.v[0] = vec3f(eye.x, eye.y, eye.z);
      app.v[1] = vec3f(look.x, look.y, look.z);
      app.v[2] = vec3f(up.x, up.y, up.z);
      app.cameraChanged = windowState->cameraChanged;
      windowState->cameraChanged = false;
    }
    // Send out the shared app state that the workers need to know, e.g. camera
    // position, if we should be quitting.
    MPI_Bcast(&app, sizeof(AppState), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (app.fbSizeChanged) {
      fb = FrameBuffer(app.fbSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
      fb.clear(OSP_FB_COLOR | OSP_FB_ACCUM);
      camera.set("aspect", static_cast<float>(app.fbSize.x) / app.fbSize.y);
      camera.commit();

      app.fbSizeChanged = false;
      if (rank == 0) {
        glViewport(0, 0, app.fbSize.x, app.fbSize.y);
      }
    }
  }
  return 0;
}


