// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2016-2018 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/utility/getEnvVar.h"
#include "ospcommon/utility/SaveImage.h"
#include "ospcommon/utility/StringManip.h"

#include "imguiViewer.h"

#include "common/sg/common/FrameBuffer.h"
#include "common/sg/visitor/GatherNodesByName.h"
#include "common/sg/visitor/GatherNodesByPosition.h"

#include "sg_imgui/ospray_sg_ui.h"

#include <imgui.h>
#include <imguifilesystem/imguifilesystem.h>

#include <unordered_map>

using std::string;
using namespace ospcommon;

namespace ospray {

  // Functions to make ImGui widgets for SG nodes /////////////////////////////

  static void sgWidget_box3f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    box3f val = node->valueAs<box3f>();
    ImGui::Text("(%f, %f, %f)", val.lower.x, val.lower.y, val.lower.z);
    ImGui::SameLine();
    ImGui::Text("(%f, %f, %f)", val.upper.x, val.upper.y, val.upper.z);
  }

  static void sgWidget_vec4f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec4f val = node->valueAs<vec4f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f, %f, %f)", val.x, val.y, val.z, val.w);
    } else if (nodeFlags & sg::NodeFlags::gui_slider) {
      if (ImGui::SliderFloat4(text.c_str(), &val.x,
                              node->min().get<vec4f>().x,
                              node->max().get<vec4f>().x))
        node->setValue(val);
    } else if (ImGui::DragFloat4(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec3f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec3f val = node->valueAs<vec3f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f, %f)", val.x, val.y, val.z);
    } else if (nodeFlags & sg::NodeFlags::gui_color) {
      if (ImGui::ColorEdit3(text.c_str(), (float*)&val.x))
        node->setValue(val);
    } else if (nodeFlags & sg::NodeFlags::gui_slider) {
      if (ImGui::SliderFloat3(text.c_str(), &val.x,
                              node->min().get<vec3f>().x,
                              node->max().get<vec3f>().x))
        node->setValue(val);
    } else if (ImGui::DragFloat3(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec3i(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec3i val = node->valueAs<vec3i>();
    ImGui::Text("(%i, %i, %i)", val.x, val.y, val.z);
  }

  static void sgWidget_vec2f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec2f val = node->valueAs<vec2f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f)", val.x, val.y);
    } else if (ImGui::DragFloat2(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec2i(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec2i val = node->valueAs<vec2i>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%i, %i)", val.x, val.y);
    } else if (ImGui::DragInt2(text.c_str(), (int*)&val.x)) {
      node->setValue(val);
    }
  }

  static void sgWidget_float(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    float val = node->valueAs<float>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("%f", val);
    } else if ((node->flags() & sg::NodeFlags::gui_slider)) {
      if (ImGui::SliderFloat(text.c_str(), &val,
                             node->min().get<float>(),
                             node->max().get<float>()))
        node->setValue(val);
    } else if (node->flags() & sg::NodeFlags::valid_min_max) {
      if (ImGui::DragFloat(text.c_str(), &val, .01f, node->min().get<float>(), node->max().get<float>()))
        node->setValue(val);
    } else if (ImGui::DragFloat(text.c_str(), &val, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_bool(const std::string &text,
                            std::shared_ptr<sg::Node> node)
  {
    bool val = node->valueAs<bool>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text(val ? "true" : "false");
    } else if (ImGui::Checkbox(text.c_str(), &val)) {
      node->setValue(val);
    }
  }

  static void sgWidget_int(const std::string &text,
                           std::shared_ptr<sg::Node> node)
  {
    int val = node->valueAs<int>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("%i", val);
    } else if ((node->flags() & sg::NodeFlags::gui_slider)) {
      if (ImGui::SliderInt(text.c_str(), &val,
                           node->min().get<int>(),
                           node->max().get<int>()))
        node->setValue(val);
    } else if (node->flags() & sg::NodeFlags::valid_min_max) {
      if (ImGui::DragInt(text.c_str(), &val, .01f, node->min().get<int>(), node->max().get<int>()))
        node->setValue(val);
    } else if (ImGui::DragInt(text.c_str(), &val)) {
      node->setValue(val);
    }
  }

  static void sgWidget_string(const std::string &text,
                              std::shared_ptr<sg::Node> node)
  {
    auto value = node->valueAs<std::string>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text(value.c_str());
    } else {
      auto whitelist = node->whitelist();
      if (!whitelist.empty()) {
        int val = -1;

        std::string list;
        for (auto it = whitelist.begin(); it != whitelist.end(); ++it) {
            auto option = *it;
            if (option.get<std::string>() == value)
                val = std::distance(whitelist.begin(), it);
            list += option.get<std::string>();
            list.push_back('\0');
        }

        // add to whitelist if not found
        if (val == -1) {
          val = whitelist.size();
          whitelist.push_back(value);
          node->setWhiteList(whitelist);
          list += value;
          list.push_back('\0');
        }

        ImGui::Combo(text.c_str(), &val, list.c_str(), whitelist.size());
        node->setValue(whitelist[val]);
      } else {
        std::vector<char> buf(value.size() + 1 + 256);
        strcpy(buf.data(), value.c_str());
        buf[value.size()] = '\0';
        if (ImGui::InputText(text.c_str(), buf.data(),
                             value.size()+256,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            node->setValue(std::string(buf.data()));
        }
      }
    }
  }

  using ParameterWidgetBuilder =
      void(*)(const std::string &, std::shared_ptr<sg::Node>);

  static std::unordered_map<std::string, ParameterWidgetBuilder>
      widgetBuilders =
      {
        {"float", sgWidget_float},
        {"int", sgWidget_int},
        {"vec2i", sgWidget_vec2i},
        {"vec2f", sgWidget_vec2f},
        {"vec3f", sgWidget_vec3f},
        {"vec3i", sgWidget_vec3i},
        {"vec4f", sgWidget_vec4f},
        {"box3f", sgWidget_box3f},
        {"string", sgWidget_string},
        {"bool", sgWidget_bool}
      };


// ImGuiViewer definitions ////////////////////////////////////////////////////

  ImGuiViewer::ImGuiViewer(const std::shared_ptr<sg::Frame> &scenegraph)
    : ImGui3DWidget(ImGui3DWidget::RESIZE_KEEPFOVY),
      scenegraph(scenegraph),
      renderer(scenegraph->child("renderer").nodeAs<sg::Renderer>()),
      renderEngine(scenegraph)
  {
    auto OSPRAY_DYNAMIC_LOADBALANCER=
      utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

    useDynamicLoadBalancer = OSPRAY_DYNAMIC_LOADBALANCER.value_or(false);

    if (useDynamicLoadBalancer)
      numPreAllocatedTiles = OSPRAY_DYNAMIC_LOADBALANCER.value();

    auto bbox = renderer->child("world").bounds();
    if (bbox.empty()) {
      bbox.lower = vec3f(-5,0,-5);
      bbox.upper = vec3f(5,10,5);
    }
    setWorldBounds(bbox);

    ospSetProgressFunc(&ospray::ImGuiViewer::progressCallbackWrapper, this);

    auto &camera = scenegraph->child("camera");
    auto pos  = camera["pos"].valueAs<vec3f>();
    auto gaze = camera["gaze"].valueAs<vec3f>();
    auto up   = camera["up"].valueAs<vec3f>();
    setViewPort(pos, gaze, up);

    // remove "gaze" as it's not an actual OSPRay parameter
    camera.remove("gaze");

    originalView = viewPort;

    transferFunctionWidget.loadColorMapPresets(
      renderer->child("transferFunctionPresets").shared_from_this()
    );

    transferFunctionWidget.setColorMapByName("Jet");
  }

  ImGuiViewer::~ImGuiViewer()
  {
    renderEngine.stop();
    ospSetProgressFunc(nullptr, nullptr);
  }

  void ImGuiViewer::startAsyncRendering()
  {
    renderEngine.start();
  }

  void ImGuiViewer::setViewportToSgCamera()
  {
    auto &camera = scenegraph->child("camera");

    auto from = camera["pos"].valueAs<vec3f>();
    auto dir  = camera["dir"].valueAs<vec3f>();
    auto up   = camera["up"].valueAs<vec3f>();
    auto dist = camera["focusDistance"].valueAs<float>();

    viewPort.from = from;
    viewPort.at   = from + (normalize(dir) * dist);
    viewPort.up   = up;

    computeFrame();
  }

  void ImGuiViewer::setDefaultViewportToCurrent()
  {
    originalView = viewPort;
  }

  void ImGuiViewer::setInitialSearchBoxText(const std::string &text)
  {
    nodeNameForSearch = text;
  }

  void ImGuiViewer::mouseButton(int button, int action, int mods)
  {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS
        && ((mods & GLFW_MOD_SHIFT) | (mods & GLFW_MOD_CONTROL))) {
      const vec2f pos(currMousePos.x / static_cast<float>(windowSize.x),
                      1.f - currMousePos.y / static_cast<float>(windowSize.y));
      renderEngine.pick(pos);
      lastPickQueryType = (mods & GLFW_MOD_SHIFT) ? PICK_CAMERA : PICK_NODE;
    }
  }

  void ImGuiViewer::reshape(const vec2i &newSize)
  {
    ImGui3DWidget::reshape(newSize);
    scenegraph->child("frameBuffer")["size"].setValue(renderSize);
    // only resize 2nd FB if needed
    //if (renderResolutionScale != navRenderResolutionScale)
      scenegraph->child("navFrameBuffer")["size"].setValue(navRenderSize);
  }

  void ImGuiViewer::keypress(char key)
  {
    switch (key) {
    case ' ':
    {
      if (renderer && renderer->hasChild("animationcontroller")) {
        bool animating =
            renderer->child("animationcontroller")["enabled"].valueAs<bool>();
        renderer->child("animationcontroller")["enabled"] = !animating;
      }
      break;
    }
    case 'R':
      toggleRenderingPaused();
      break;
    case '!':
      saveScreenshot = true;
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
    case 'd':
      resetDefaultView();
      break;
    case 'p':
      printViewport();
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
    viewPort.modified = true;
  }

  void ImGuiViewer::resetDefaultView()
  {
    auto &world = renderer->child("world");
    auto bbox = world.bounds();
    vec3f diag = bbox.size();
    diag = max(diag, vec3f(0.3f * length(diag)));

    auto gaze = ospcommon::center(bbox);
    auto pos = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
    auto up = vec3f(0.f, 1.f, 0.f);

    auto &camera = scenegraph->child("camera");
    camera["pos"] = pos;
    camera["dir"] = normalize(gaze - pos);
    camera["up"] = up;

    setViewPort(pos, gaze, up);
    originalView = viewPort;
  }

  void ImGuiViewer::printViewport()
  {
    printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
           viewPort.from.x, viewPort.from.y, viewPort.from.z,
           viewPort.up.x,   viewPort.up.y,   viewPort.up.z,
           viewPort.at.x,   viewPort.at.y,   viewPort.at.z);
    fflush(stdout);
  }

  void ImGuiViewer::toggleRenderingPaused()
  {
    renderingPaused = !renderingPaused;
    renderingPaused ? renderEngine.stop() : renderEngine.start();
  }

  void ImGuiViewer::display()
  {
    if (renderEngine.hasNewPickResult()) {
      auto picked = renderEngine.getPickResult();
      if (picked.hit) {
        if (lastPickQueryType == PICK_NODE) {
          sg::GatherNodesByPosition visitor((vec3f&)picked.position);
          scenegraph->traverse(visitor);
          collectedNodesFromSearch = visitor.results();
        } else {
          // No conversion operator or ctor??
          viewPort.at.x = picked.position.x;
          viewPort.at.y = picked.position.y;
          viewPort.at.z = picked.position.z;
          viewPort.modified = true;
        }
      }
    }

    if (viewPort.modified) {
      auto &camera = scenegraph->child("camera");
      auto dir = viewPort.at - viewPort.from;
      if (camera.hasChild("focusdistance"))
        camera["focusdistance"] = length(dir);
      dir = normalize(dir);
      camera["dir"] = dir;
      camera["pos"] = viewPort.from;
      camera["up"]  = viewPort.up;
      camera.markAsModified();

      // don't cancel the first frame, otherwise it is hard to navigate
      if (scenegraph->frameId() > 0 && cancelFrameOnInteraction) {
        cancelRendering = true;
        renderEngine.setFrameCancelled();
      }

      viewPort.modified = false;
    }

    renderFPS = renderEngine.lastFrameFps();
    renderFPSsmoothed = renderEngine.lastFrameFpsSmoothed();

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      auto fbSize = mappedFB.size();
      auto fbData = mappedFB.data();
      GLenum texelType;
      std::string filename("ospexampleviewer");
      switch (mappedFB.format()) {
        default: /* fallthrough */
        case OSP_FB_NONE:
          fbData = nullptr;
          break;
        case OSP_FB_RGBA8: /* fallthrough */
        case OSP_FB_SRGBA:
          texelType = GL_UNSIGNED_BYTE;
          if (saveScreenshot) {
            filename += ".ppm";
            utility::writePPM(filename, fbSize.x, fbSize.y, (uint32_t*)fbData);
          }
          break;
        case OSP_FB_RGBA32F:
          texelType = GL_FLOAT;
          if (saveScreenshot) {
            filename += ".pfm";
            utility::writePFM(filename, fbSize.x, fbSize.y, (vec4f*)fbData);
          }
          break;
      }

      // update/upload fbTexture
      if (fbData) {
        fbAspect = fbSize.x/float(fbSize.y);
        glBindTexture(GL_TEXTURE_2D, fbTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbSize.x, fbSize.y, 0, GL_RGBA,
            texelType, fbData);
      } else
        fbAspect = 1.f;

      if (saveScreenshot) {
        std::cout << "saved current frame to '" << filename << "'" << std::endl;
        saveScreenshot = false;
      }

      renderEngine.unmapFramebuffer();
    }

    // set border color TODO maybe move to application
    vec4f texBorderCol(0.f); // default black
    // TODO be more sophisticated (depending on renderer type, fb mode (sRGB))
    if (renderer->child("useBackplate").valueAs<bool>()) {
      auto col = renderer->child("bgColor").valueAs<vec3f>();
      const float g = 1.f/2.2f;
      texBorderCol = vec4f(powf(col.x, g), powf(col.y, g), powf(col.z, g), 0.f);
    }
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &texBorderCol[0]);

    ImGui3DWidget::display();

    lastTotalTime = ImGui3DWidget::totalTime;
    lastGUITime = ImGui3DWidget::guiTime;
    lastDisplayTime = ImGui3DWidget::displayTime;
  }

  void ImGuiViewer::buildGui()
  {
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;

    ImGui::Begin("Viewer Controls: press 'g' to show/hide", nullptr, flags);
    ImGui::SetWindowFontScale(0.5f*fontScale);

    guiMenu();

#if 0 // NOTE(jda) - enable to see ImGui example window
    static bool demo_window = true;
    ImGui::ShowTestWindow(&demo_window);
#endif

    guiRenderStats();
    guiRenderCustomWidgets();
    guiFindNode();
    guiTransferFunction();

    if (ImGui::CollapsingHeader("SceneGraph", "SceneGraph", true, false))
      guiSGTree("root", scenegraph);

    ImGui::End();
  }

  void ImGuiViewer::guiMenu()
  {
    if (ImGui::BeginMenuBar()) {
      guiMenuApp();
      guiMenuView();
      guiMenuMPI();

      ImGui::EndMenuBar();
    }
  }

  void ImGuiViewer::guiMenuApp()
  {
    if (ImGui::BeginMenu("App")) {

      ImGui::Checkbox("Auto-Rotate", &animating);

      bool paused = renderingPaused;
      if (ImGui::Checkbox("Pause Rendering", &paused))
        toggleRenderingPaused();

      ImGui::Checkbox("Interaction Cancels Frame", &cancelFrameOnInteraction);

      ImGui::Separator();

      if (ImGui::BeginMenu("Scale Resolution")) {
        float scale = renderResolutionScale;
        if (ImGui::MenuItem("0.25x")) renderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x")) renderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x")) renderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x")) renderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x")) renderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x")) renderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x")) renderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &renderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != renderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scale Resolution while Navigating")) {
        float scale = navRenderResolutionScale;
        if (ImGui::MenuItem("0.25x")) navRenderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x")) navRenderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x")) navRenderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x")) navRenderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x")) navRenderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x")) navRenderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x")) navRenderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &navRenderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != navRenderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      ImGui::Separator();

      if (ImGui::BeginMenu("Aspect Control")) {
        const float aspect = fixedRenderAspect;
        if (ImGui::MenuItem("Lock"))
          fixedRenderAspect = (float)windowSize.x / windowSize.y;
        if (ImGui::MenuItem("Unlock")) fixedRenderAspect = 0.f;
        ImGui::InputFloat("Set", &fixedRenderAspect);
        fixedRenderAspect = std::max(fixedRenderAspect, 0.f);

        if (aspect != fixedRenderAspect) {
          if (fixedRenderAspect > 0.f)
            resizeMode = RESIZE_LETTERBOX;
          else
            resizeMode = RESIZE_KEEPFOVY;
          reshape(windowSize);
        }

        ImGui::EndMenu();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Take Screenshot"))
          saveScreenshot = true;

      ImGui::Separator();

      if (ImGui::MenuItem("Quit")) {
        renderEngine.stop();
        std::exit(0);
      }

      ImGui::EndMenu();
    }
  }

  void ImGuiViewer::guiMenuView()
  {
    if (ImGui::BeginMenu("View")) {
      bool orbitMode = (manipulator == inspectCenterManipulator.get());
      bool flyMode   = (manipulator == moveModeManipulator.get());

      if (ImGui::Checkbox("Orbit Camera Mode", &orbitMode))
        manipulator = inspectCenterManipulator.get();

      if (orbitMode) ImGui::Checkbox("Anchor 'Up' Direction", &upAnchored);

      if (ImGui::Checkbox("Fly Camera Mode", &flyMode))
        manipulator = moveModeManipulator.get();

      if (ImGui::MenuItem("Reset View to Default")) resetView();
      if (ImGui::MenuItem("Set View as Default")) setDefaultViewportToCurrent();
      if (ImGui::MenuItem("Create Default View")) resetDefaultView();
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Accumulation")) viewPort.modified = true;
      if (ImGui::MenuItem("Print View")) printViewport();

      ImGui::InputFloat("Motion Speed", &motionSpeed);

      ImGui::EndMenu();
    }
  }

  void ImGuiViewer::guiMenuMPI()
  {
    if (ImGui::BeginMenu("MPI Extras")) {
      if (ImGui::Checkbox("Use Dynamic Load Balancer",
                          &useDynamicLoadBalancer)) {
        setCurrentDeviceParameter("dynamicLoadBalancer",
                                  useDynamicLoadBalancer);
        viewPort.modified = true;
      }

      if (useDynamicLoadBalancer) {
        if (ImGui::InputInt("PreAllocated Tiles", &numPreAllocatedTiles)) {
          setCurrentDeviceParameter("preAllocatedTiles",
                                    numPreAllocatedTiles);
        }
      }

      ImGui::EndMenu();
    }
  }

  void ImGuiViewer::guiRenderStats()
  {
    if (ImGui::CollapsingHeader("Rendering Statistics", "Rendering Statistics",
                                true, false)) {
      ImGui::NewLine();
      if (renderFPS > 1.f) {
        ImGui::Text("OSPRay render rate: %.1f fps", renderFPS);
      } else {
        ImGui::Text("OSPRay render time: %.1f sec. Frame progress: ", 1.f/renderFPS);
        ImGui::SameLine();
        ImGui::ProgressBar(frameProgress);
      }
      ImGui::Text("  Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1f ms", lastTotalTime*1000.f);
      ImGui::Text("  GUI time: %.1f ms", lastGUITime*1000.f);
      ImGui::Text("  display pixel time: %.1f ms", lastDisplayTime*1000.f);
      auto variance = renderer->getLastVariance();
      ImGui::Text("Variance: %.3f", variance);

      auto eta = scenegraph->estimatedSeconds();
      if (std::isfinite(eta)) {
        auto sec = scenegraph->elapsedSeconds();
        ImGui::SameLine();
        ImGui::Text(" Total progress: ");

        char str[100];
        if (sec < eta)
          snprintf(str, sizeof(str), "%.1f s / %.1f s", sec, eta);
        else
          snprintf(str, sizeof(str), "%.1f s", sec);

        ImGui::SameLine();
        ImGui::ProgressBar(sec/eta, ImVec2(-1,0), str);
      }

      ImGui::NewLine();
    }
  }

  void ImGuiViewer::guiRenderCustomWidgets()
  {
    for (const auto &p : customPanes) {
      if (ImGui::CollapsingHeader(p.first.c_str(), p.first.c_str(),
                                  true, false)) {
        p.second(*scenegraph);
      }
    }
  }

  void ImGuiViewer::guiTransferFunction()
  {
    if (!renderer->hasChild("transferFunctions"))
      return;
    auto tfNodes = renderer->child("transferFunctions").children();
    if (tfNodes.empty())
      return;
    std::vector<std::shared_ptr<sg::Node>> transferFunctions;
    std::shared_ptr<sg::Node> transferFunction = nullptr;
    std::string tfNodeListString;
    for (auto& tf : tfNodes)
    {
      tfNodeListString += tf.second->name() + '\0';
      transferFunctions.push_back(tf.second);
    }
    if (ImGui::CollapsingHeader("TransferFunctions", "TransferFunctions", true, true))
    {
      ImGui::Combo("transferFunction", &transferFunctionSelection, tfNodeListString.c_str(), tfNodes.size());
      if (transferFunctionSelection >= 0 && transferFunctionSelection < transferFunctions.size())
        transferFunction = transferFunctions[transferFunctionSelection];
      if (transferFunction) {
        transferFunctionWidget.setSGTF(transferFunction->nodeAs<sg::TransferFunction>());
        transferFunctionWidget.render();
        transferFunctionWidget.drawUi();
      }
    }
  }

  void ImGuiViewer::guiFindNode()
  {
    if (ImGui::CollapsingHeader("Find Node", "Find Node", true, false)) {
      ImGui::NewLine();

      std::array<char, 512> buf;
      strcpy(buf.data(), nodeNameForSearch.c_str());

      ImGui::Text("Search for node:");
      ImGui::SameLine();

      ImGui::InputText("", buf.data(), buf.size(),
                       ImGuiInputTextFlags_EnterReturnsTrue);

      std::string textBoxValue = buf.data();

      bool updateSearchResults = (nodeNameForSearch != textBoxValue);
      if (updateSearchResults) {
        nodeNameForSearch = textBoxValue;
        bool doSearch = !nodeNameForSearch.empty();
        if (doSearch) {
          sg::GatherNodesByName visitor(nodeNameForSearch);
          scenegraph->traverse(visitor);
          collectedNodesFromSearch = visitor.results();
        } else {
          collectedNodesFromSearch.clear();
        }
      }

      if (nodeNameForSearch.empty()) {
        ImGui::Text("search for: N/A");
      } else {
        const auto verifyTextLabel = std::string("search for: ")
                                     + nodeNameForSearch;
        ImGui::Text(verifyTextLabel.c_str());
      }

      if (ImGui::Button("Clear")) {
        collectedNodesFromSearch.clear();
        nodeNameForSearch.clear();
      }

      ImGui::NewLine();

      guiSearchSGNodes();
    }
  }

  void ImGuiViewer::guiSearchSGNodes()
  {
    if (collectedNodesFromSearch.empty()) {
      if (!nodeNameForSearch.empty()) {
        std::string text = "No nodes found with name '";
        text += nodeNameForSearch;
        text += "'";
        ImGui::Text(text.c_str());
      }
    } else {
      for (auto &node : collectedNodesFromSearch) {
        guiSGTree("", node);
        ImGui::Separator();
      }
    }
  }

  void ImGuiViewer::setCurrentDeviceParameter(const std::string &param,
                                              int value)
  {
    renderEngine.stop();

    auto device = ospGetCurrentDevice();
    if (device == nullptr)
      throw std::runtime_error("FATAL: could not get current OSPDevice!");

    ospDeviceSet1i(device, param.c_str(), value);
    ospDeviceCommit(device);

    if (!renderingPaused)
      renderEngine.start();
  }

  void ImGuiViewer::setColorMap(std::string name)
  {
    transferFunctionWidget.setColorMapByName(name, true);
  }

  int ImGuiViewer::progressCallback(const float progress)
  {
    frameProgress = progress;

    // one-shot cancel
    return !cancelRendering.exchange(false);
  }

  int ImGuiViewer::progressCallbackWrapper(void * ptr, const float progress)
  {
    return ((ImGuiViewer*)ptr)->progressCallback(progress);
  }

} // ::ospray
