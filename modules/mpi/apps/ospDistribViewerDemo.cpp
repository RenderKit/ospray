#include <random>
#include <array>
#include <chrono>
#include <GLFW/glfw3.h>
#include <mpiCommon/MPICommon.h>
#include <mpi.h>
#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Data.h>
#include <ospray/ospray_cpp/Device.h>
#include <ospray/ospray_cpp/FrameBuffer.h>
#include <ospray/ospray_cpp/Geometry.h>
#include <ospray/ospray_cpp/Renderer.h>
#include <ospray/ospray_cpp/TransferFunction.h>
#include <ospray/ospray_cpp/Volume.h>
#include <ospray/ospray_cpp/Model.h>
#include "generateSciVis.h"

using namespace ospray::cpp;
using namespace ospcommon;

struct Sphere {
  vec3f org;
  int colorID{0};
};

// Struct for bcasting out the camera change info and general app state
struct AppState {
  // eye pos, look dir, up dir
  std::array<vec3f, 3> v;
  vec2i fbSize;
  bool cameraChanged, quit, fbSizeChanged;

  AppState() : fbSize(1024), cameraChanged(false), quit(false),
  fbSizeChanged(false)
  {}
};
// Extra junk we need in GLFW callbacks
struct WindowState {
  AffineSpace3f camera;
  vec2f prevMouse;
  bool cameraChanged;
  float frameDelta;
  AppState &app;

  WindowState(AppState &app)
    : camera(AffineSpace3f::lookat(vec3f(0, 0, 2), vec3f(0), vec3f(0, 1, 0))),
    prevMouse(-1), cameraChanged(false), frameDelta(0), app(app)
  {}
};

void keyCallback(GLFWwindow *window, int key, int, int action, int) {
  WindowState *state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;
      case GLFW_KEY_R:
        //state->camera.reset();
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
    if (leftDown) {
      //state->camera.rotate(mouse, mouse - state->prevMouse, state->frameDelta);
      //state->cameraChanged = true;
    } else if (rightDown) {
      //state->camera.zoom(mouse.y - state->prevMouse.y, state->frameDelta);
      //state->cameraChanged = true;
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
  auto volume = gensv::makeVolume();
  model.addVolume(volume.first);
  auto spheres = gensv::makeSpheres(volume.second, 100, 0.01);
  model.addGeometry(spheres);

  std::vector<box3f> regions{volume.second};
  ospray::cpp::Data regionData(regions.size() * 2, OSP_FLOAT3,
      regions.data());
  model.set("regions", regionData);

  model.commit();

  Camera camera("perspective");
  const vec3f pos(0, 1.5, 2);
  camera.set("pos", pos);
  camera.set("dir", vec3f(0.5) - pos);
  camera.set("up", vec3f(0, 1, 0));
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
    windowState = std::make_shared<WindowState>(app);
    windowState->frameDelta = 0.16;

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
    renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);

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

      /*
       const vec3f eye = windowState->camera.eyePos();
       const vec3f look = windowState->camera.lookDir();
       const vec3f up = windowState->camera.upDir();
       app.v[0] = vec3f(eye.x, eye.y, eye.z);
       app.v[1] = vec3f(look.x, look.y, look.z);
       app.v[2] = vec3f(up.x, up.y, up.z);
       app.cameraChanged = windowState->cameraChanged;
       */
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


