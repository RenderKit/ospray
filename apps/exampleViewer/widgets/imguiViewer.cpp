// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2016 Intel Corporation                                         //
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

#include "imguiViewer.h"

#include <imgui.h>

using std::string;
using namespace ospcommon;

// Static local helper functions //////////////////////////////////////////////

// helper function to write the rendered image as PPM file
static void writePPM(const string &fileName, const int sizeX, const int sizeY,
                     const uint32_t *pixel)
{
  FILE *file = fopen(fileName.c_str(), "wb");
  fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
  unsigned char *out = (unsigned char *)alloca(3*sizeX);
  for (int y = 0; y < sizeY; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
    for (int x = 0; x < sizeX; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x + 2];
    }
    fwrite(out, 3*sizeX, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}

// ImGuiViewer definitions ////////////////////////////////////////////////////

namespace ospray {

  ImGuiViewer::ImGuiViewer(const std::deque<box3f> &worldBounds,
                           const std::deque<cpp::Model> &model,
                           cpp::Renderer renderer,
                           cpp::Camera camera)
    : ImGuiViewer(worldBounds, model, renderer,
                  cpp::Renderer(), cpp::FrameBuffer(), camera)
  {
  }

  ImGuiViewer::ImGuiViewer(const std::deque<box3f> &worldBounds,
                           const std::deque<cpp::Model> &model,
                           cpp::Renderer renderer,
                           cpp::Renderer rendererDW,
                           cpp::FrameBuffer frameBufferDW,
                           cpp::Camera camera)
    : ImGui3DWidget(ImGui3DWidget::FRAMEBUFFER_NONE),
      sceneModels(model),
      worldBounds(worldBounds),
      camera(camera),
      renderer(renderer),
      rendererDW(rendererDW),
      frameBufferDW(frameBufferDW)
  {
    if (!worldBounds.empty())
      setWorldBounds(worldBounds[0]);

    renderer.set("model",  sceneModels[0]);
    renderer.set("camera", camera);
    renderer.set("bgColor", 1.f, 1.f, 1.f, 1.f);

    if (rendererDW) {
      rendererDW.set("model",  sceneModels[0]);
      rendererDW.set("camera", camera);
      rendererDW.set("bgColor", 1.f, 1.f, 1.f, 1.f);
    }
    renderEngine.setRenderer(renderer, rendererDW, frameBufferDW);
    renderEngine.setFbSize({1024, 768});

    renderEngine.scheduleObjectCommit(renderer);
    if (rendererDW)
      renderEngine.scheduleObjectCommit(rendererDW);
    renderEngine.start();

    frameTimer = ospcommon::getSysTime();
    originalView = viewPort;
  }

  ImGuiViewer::~ImGuiViewer()
  {
    renderEngine.stop();
  }

  void ImGuiViewer::setRenderer(OSPRenderer renderer, 
                                OSPRenderer rendererDW,
                                OSPFrameBuffer frameBufferDW)
  {
    this->renderer = renderer;
    this->rendererDW = rendererDW;
    this->frameBufferDW = frameBufferDW;
    renderEngine.setRenderer(renderer,rendererDW,frameBufferDW);
  }

  void ImGuiViewer::reshape(const vec2i &newSize)
  {
    ImGui3DWidget::reshape(newSize);
    windowSize = newSize;

    viewPort.modified = true;

    renderEngine.setFbSize(newSize);
    pixelBuffer.resize(newSize.x * newSize.y);
  }

  void ImGuiViewer::keypress(char key)
  {
    switch (key) {
    case ' ':
      animationPaused = !animationPaused;
      break;
    case '<':
      animationFrameDelta = max(animationFrameDelta-0.01, 0.0001);
      break;
    case '>':
      animationFrameDelta = min(animationFrameDelta+0.01, 1.0);
      break;
    case '=':
    case '+':
      motionSpeed *= 1.5f;
      std::cout << "new motion speed: " << motionSpeed << std::endl;
      break;
    case '-':
      motionSpeed /= 1.5f;
      std::cout << "new motion speed: " << motionSpeed << std::endl;
      break;
    case 'R':
      toggleRenderingPaused();
      break;
    case '!':
      saveScreenshot("ospimguiviewer");
      break;
    case 'X':
      if (viewPort.up == vec3f(1,0,0) || viewPort.up == vec3f(-1.f,0,0)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(1,0,0);
      }
      viewPort.modified = true;
      break;
    case 'Y':
      if (viewPort.up == vec3f(0,1,0) || viewPort.up == vec3f(0,-1.f,0)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(0,1,0);
      }
      viewPort.modified = true;
      break;
    case 'Z':
      if (viewPort.up == vec3f(0,0,1) || viewPort.up == vec3f(0,0,-1.f)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(0,0,1);
      }
      viewPort.modified = true;
      break;
    case 'c':
      viewPort.modified = true;//Reset accumulation
      break;
    case 'r':
      resetView();
      break;
    case 'p':
      printViewport();
      break;
    case 27 /*ESC*/:
    case 'q':
    case 'Q':
      renderEngine.stop();
      std::exit(0);
    break;
    default:
      ImGui3DWidget::keypress(key);
    }
  }

  void ImGuiViewer::resetView()
  {
    auto oldAspect = viewPort.aspect;
    viewPort = originalView;
    viewPort.aspect = oldAspect;
  }

  void ImGuiViewer::printViewport()
  {
    printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
           viewPort.from.x, viewPort.from.y, viewPort.from.z,
           viewPort.up.x,   viewPort.up.y,   viewPort.up.z,
           viewPort.at.x,   viewPort.at.y,   viewPort.at.z);
    fflush(stdout);
  }

  void ImGuiViewer::saveScreenshot(const std::string &basename)
  {
    writePPM(basename + ".ppm", windowSize.x, windowSize.y, pixelBuffer.data());
    std::cout << "saved current frame to '" << basename << ".ppm'" << std::endl;
  }

  void ImGuiViewer::toggleRenderingPaused()
  {
    renderingPaused = !renderingPaused;
    renderingPaused ? renderEngine.stop() : renderEngine.start();
  }

  void ImGuiViewer::setWorldBounds(const box3f &worldBounds) 
  {
    ImGui3DWidget::setWorldBounds(worldBounds);
    aoDistance = (worldBounds.upper.x - worldBounds.lower.x)/4.f;
    renderer.set("aoDistance", aoDistance);
    if (rendererDW)
      rendererDW.set("aoDistance", aoDistance);
    renderEngine.scheduleObjectCommit(renderer);
  }

  void ImGuiViewer::display()
  {
    updateAnimation(ospcommon::getSysTime()-frameTimer);
    frameTimer = ospcommon::getSysTime();

    if (viewPort.modified) {
      Assert2(camera.handle(),"ospray camera is null");
      camera.set("pos", viewPort.from);
      auto dir = viewPort.at - viewPort.from;
      camera.set("dir", dir);
      camera.set("up", viewPort.up);
      camera.set("aspect", viewPort.aspect);
      camera.set("fovy", viewPort.openingAngle);

      viewPort.modified = false;
      renderEngine.scheduleObjectCommit(camera);
    }

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      size_t nPixels = windowSize.x * windowSize.y;

      if (mappedFB.size() == nPixels) {
        auto *srcPixels = mappedFB.data();
        auto *dstPixels = pixelBuffer.data();
        memcpy(dstPixels, srcPixels, nPixels * sizeof(uint32_t));
        lastFrameFPS = renderEngine.lastFrameFps();
        renderTime = 1.f/lastFrameFPS;
      }

      renderEngine.unmapFramebuffer();
    }

    ucharFB = pixelBuffer.data();
    frameBufferMode = ImGui3DWidget::FRAMEBUFFER_UCHAR;
    ImGui3DWidget::display();

    // that pointer is no longer valid, so set it to null
    ucharFB = nullptr;
  }

  void ImGuiViewer::updateAnimation(double deltaSeconds)
  {
    if (sceneModels.size() < 2)
      return;
    if (animationPaused)
      return;
    animationTimer += deltaSeconds;
    int framesSize = sceneModels.size();
    const int frameStart = (lockFirstAnimationFrame ? 1 : 0);
    if (lockFirstAnimationFrame)
      framesSize--;

    if (animationTimer > animationFrameDelta)
      {
        animationFrameId++;

        //set animation time to remainder off of delta
        animationTimer -= int(animationTimer/deltaSeconds) * deltaSeconds;

        size_t dataFrameId = animationFrameId%framesSize+frameStart;
        if (lockFirstAnimationFrame)
          {
            ospcommon::affine3f xfm = ospcommon::one;
            xfm *= ospcommon::affine3f::translate(translate)
              * ospcommon::affine3f::scale(scale);
            OSPGeometry dynInst =
              ospNewInstance((OSPModel)sceneModels[dataFrameId].object(),
                             (osp::affine3f&)xfm);
            ospray::cpp::Model worldModel = ospNewModel();
            ospcommon::affine3f staticXFM = ospcommon::one;
            OSPGeometry staticInst =
              ospNewInstance((OSPModel)sceneModels[0].object(),
                             (osp::affine3f&)staticXFM);
            //Carson: TODO: creating new world model every frame unecessary
            worldModel.addGeometry(staticInst);
            worldModel.addGeometry(dynInst);
            renderEngine.scheduleObjectCommit(worldModel);
            renderer.set("model",  worldModel);
            if (rendererDW)
              rendererDW.set("model",  worldModel);
          }
        else
          {
            renderer.set("model",  sceneModels[dataFrameId]);
            if (rendererDW)
              rendererDW.set("model",  sceneModels[dataFrameId]);
          }

        renderEngine.scheduleObjectCommit(renderer);
        if (rendererDW)
          renderEngine.scheduleObjectCommit(rendererDW);
      }
  }

  void ImGuiViewer::buildGui()
  {
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;

    static bool demo_window = false;

    ImGui::Begin("Viewer Controls: press 'g' to show/hide", nullptr, flags);
    ImGui::SetWindowFontScale(0.5f*fontScale);

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("App")) {
#if 0
          ImGui::Checkbox("Show ImGui Demo Window", &demo_window);
#endif

        ImGui::Checkbox("Auto-Rotate", &animating);

        bool paused = renderingPaused;
        if (ImGui::Checkbox("Pause Rendering", &paused))
          toggleRenderingPaused();

        if (ImGui::MenuItem("Take Screenshot"))
          saveScreenshot("ospimguiviewer");

        if (ImGui::MenuItem("Quit")) {
          renderEngine.stop();
          std::exit(0);
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        bool orbitMode = (manipulator == inspectCenterManipulator);
        bool flyMode   = (manipulator == moveModeManipulator);

        if (ImGui::Checkbox("Orbit Camera Mode", &orbitMode)) {
          manipulator = inspectCenterManipulator;
        }
        if (ImGui::Checkbox("Fly Camera Mode", &flyMode)) {
          manipulator = moveModeManipulator;
        }

        if (ImGui::MenuItem("Reset View")) resetView();
        if (ImGui::MenuItem("Reset Accumulation")) viewPort.modified = true;
        if (ImGui::MenuItem("Print View")) printViewport();

        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }

    if (demo_window) ImGui::ShowTestWindow(&demo_window);

    if (ImGui::CollapsingHeader("FPS Statistics", "FPS Statistics",
                                true, true)) {
      ImGui::NewLine();
      ImGui::Text("OSPRay render rate: %.1f FPS", lastFrameFPS);
      ImGui::Text("  GUI display rate: %.1f FPS", ImGui::GetIO().Framerate);
      ImGui::NewLine();
    }

    if (ImGui::CollapsingHeader("Renderer Parameters")) {
      bool renderer_changed = false;

      static int numThreads = -1;
      if (ImGui::InputInt("# threads", &numThreads, 1)) {
        renderEngine.stop();
        renderEngine.start(numThreads);
        renderer_changed = true;
      }

      static int ao = 1;
      if (ImGui::SliderInt("aoSamples", &ao, 0, 32)) {
        renderer.set("aoSamples", ao);
        if (rendererDW)
          rendererDW.set("aoSamples", ao);

        renderer_changed = true;
      }

      if (ImGui::InputFloat("aoDistance", &aoDistance)) {
        renderer.set("aoDistance", aoDistance);
        if (rendererDW)
          rendererDW.set("aoDistance", aoDistance);
        renderer_changed = true;
      }

      static bool ao_transparency = false;
      if (ImGui::Checkbox("ao transparency", &ao_transparency)) {
        renderer.set("aoTransparencyEnabled", int(ao_transparency));
        if (rendererDW)
          rendererDW.set("aoTransparencyEnabled", int(ao_transparency));
        renderer_changed = true;
      }

      static bool shadows = true;
      if (ImGui::Checkbox("shadows", &shadows)) {
        renderer.set("shadowsEnabled", int(shadows));
        if (rendererDW)
          rendererDW.set("shadowsEnabled", int(shadows));
        renderer_changed = true;
      }

      static bool singleSidedLighting = true;
      if (ImGui::Checkbox("single sided lighting", &singleSidedLighting)) {
        renderer.set("oneSidedLighting", int(singleSidedLighting));
        if (rendererDW)
          rendererDW.set("oneSidedLighting", int(singleSidedLighting));
        renderer_changed = true;
      }

      static int exponent = -6;
      if (ImGui::SliderInt("ray epsilon (exponent)", &exponent, -10, 2)) {
        renderer.set("epsilon", ospcommon::pow(10.f, (float)exponent));
        if (rendererDW)
          rendererDW.set("epsilon", ospcommon::pow(10.f, (float)exponent));
        renderer_changed = true;
      }

      static int spp = 1;
      if (ImGui::SliderInt("spp", &spp, -4, 16)) {
        renderer.set("spp", spp);
        if (rendererDW)
          rendererDW.set("spp", spp);
        renderer_changed = true;
      }

      static float varianceThreshold = 0.f;
      if (ImGui::InputFloat("variance threshold", &varianceThreshold)) {
        renderer.set("varianceThreshold", varianceThreshold);
        renderer_changed = true;
      }

      static ImVec4 bg_color = ImColor(255, 255, 255);
      if (ImGui::ColorEdit3("bg_color", (float*)&bg_color)) {
        renderer.set("bgColor", bg_color.x, bg_color.y, bg_color.z);
        if (rendererDW)
          rendererDW.set("bgColor", bg_color.x, bg_color.y, bg_color.z);
        renderer_changed = true;
      }

      if (renderer_changed) {
        renderEngine.scheduleObjectCommit(renderer);
        if (rendererDW)
        renderEngine.scheduleObjectCommit(rendererDW);
      }
    }

    ImGui::End();
  }

}// namepace ospray
