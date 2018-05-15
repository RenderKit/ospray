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
#include "transferFunction.h"

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

  static void sgWidget_TransferFunction(
    const std::string &text,
    std::shared_ptr<sg::Node> node)
  {
    if (!node->hasChild("transferFunctionWidget")) {
      std::shared_ptr<sg::TransferFunction> tfn =
        std::dynamic_pointer_cast<sg::TransferFunction>(node);

      node->createChildWithValue("transferFunctionWidget","Node",
                                 TransferFunction(tfn));
    }

    auto &tfnWidget =
      node->child("transferFunctionWidget").valueAs<TransferFunction>();

    static bool show_editor = true;
    ImGui::Checkbox("show_editor", &show_editor);

    if (show_editor) {
      tfnWidget.render();
      tfnWidget.drawUi();
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
        {"box3f", sgWidget_box3f},
        {"string", sgWidget_string},
        {"bool", sgWidget_bool},
        {"TransferFunction", sgWidget_TransferFunction}
      };


// ImGuiViewer definitions ////////////////////////////////////////////////////

  ImGuiViewer::ImGuiViewer(const std::shared_ptr<sg::Node> &scenegraph)
    : ImGuiViewer(scenegraph->nodeAs<sg::Renderer>(), nullptr)
  {}

  ImGuiViewer::ImGuiViewer(const std::shared_ptr<sg::Renderer> &scenegraph,
                           const std::shared_ptr<sg::Renderer> &scenegraphDW)
    : ImGui3DWidget(ImGui3DWidget::FRAMEBUFFER_NONE),
      scenegraph(scenegraph),
      scenegraphDW(scenegraphDW),
      renderEngine(scenegraph, scenegraphDW)
  {
    auto OSPRAY_DYNAMIC_LOADBALANCER=
      utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

    useDynamicLoadBalancer = OSPRAY_DYNAMIC_LOADBALANCER.value_or(false);

    if (useDynamicLoadBalancer)
      numPreAllocatedTiles = OSPRAY_DYNAMIC_LOADBALANCER.value();

    auto bbox = scenegraph->child("world").bounds();
    if (bbox.empty()) {
      bbox.lower = vec3f(-5,0,-5);
      bbox.upper = vec3f(5,10,5);
    }
    setWorldBounds(bbox);
    renderEngine.setFbSize({1024, 768});

    renderEngine.start();

    auto &camera = scenegraph->child("camera");
    auto pos  = camera["pos"].valueAs<vec3f>();
    auto gaze = camera["gaze"].valueAs<vec3f>();
    auto up   = camera["up"].valueAs<vec3f>();
    setViewPort(pos, gaze, up);

    // remove "gaze" as it's not an actual OSPRay parameter
    camera.remove("gaze");

    originalView = viewPort;
  }

  ImGuiViewer::~ImGuiViewer()
  {
    renderEngine.stop();
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
    windowSize = newSize;

    viewPort.modified = true;

    renderEngine.setFbSize(newSize);
    scenegraph->child("frameBuffer")["size"].setValue(newSize);

    pixelBuffer.resize(newSize.x * newSize.y);
  }

  void ImGuiViewer::keypress(char key)
  {
    switch (key) {
    case ' ':
    {
      if (scenegraph && scenegraph->hasChild("animationcontroller")) {
        bool animating =
            scenegraph->child("animationcontroller")["enabled"].valueAs<bool>();
        scenegraph->child("animationcontroller")["enabled"] = !animating;
      }
      break;
    }
    case 'R':
      toggleRenderingPaused();
      break;
    case '!':
      saveScreenshot("ospexampleviewer");
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
  }

  void ImGuiViewer::resetDefaultView()
  {
    auto &renderer = *scenegraph;

    auto &world = renderer["world"];
    auto bbox = world.bounds();
    vec3f diag = bbox.size();
    diag = max(diag, vec3f(0.3f * length(diag)));

    auto gaze = ospcommon::center(bbox);
    auto pos = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
    auto up = vec3f(0.f, 1.f, 0.f);

    auto &camera = renderer["camera"];
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

  void ImGuiViewer::saveScreenshot(const std::string &basename)
  {
    utility::writePPM(basename + ".ppm",
                      windowSize.x, windowSize.y, pixelBuffer.data());
    std::cout << "saved current frame to '" << basename << ".ppm'" << std::endl;
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

      if (scenegraphDW.get()) {
        auto &camera = scenegraphDW->child("camera");
        camera["dir"] = dir;
        camera["pos"] = viewPort.from;
        camera["up"]  = viewPort.up;
        camera.markAsModified();
      }

      viewPort.modified = false;
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

    lastTotalTime = ImGui3DWidget::totalTime;
    lastGUITime = ImGui3DWidget::guiTime;
    lastDisplayTime = ImGui3DWidget::displayTime;

    // that pointer is no longer valid, so set it to null
    ucharFB = nullptr;
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
    guiFindNode();

    if (ImGui::CollapsingHeader("SceneGraph", "SceneGraph", true, true))
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

      if (ImGui::MenuItem("Take Screenshot"))
          saveScreenshot("ospexampleviewer");

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

      if (ImGui::MenuItem("Reset View")) resetView();
      if (ImGui::MenuItem("Create Default View")) resetDefaultView();
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
      ImGui::Text("OSPRay render rate: %.1f fps", lastFrameFPS);
      ImGui::Text("  Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1f ms", lastTotalTime*1000.f);
      ImGui::Text("  GUI time: %.1f ms", lastGUITime*1000.f);
      ImGui::Text("  display pixel time: %.1f ms", lastDisplayTime*1000.f);
      ImGui::Text("Variance: %.3f", renderEngine.getLastVariance());
      ImGui::NewLine();
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

  void ImGuiViewer::guiSingleNode(const std::string &baseText,
                                  std::shared_ptr<sg::Node> node)
  {
    std::string text = baseText;

    auto fcn = widgetBuilders[node->type()];

    if (fcn) {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      text = "##" + std::to_string(node->uniqueID());

      fcn(text, node);
    } else if (!node->hasChildren()) {
      text += node->type();
      ImGui::Text(text.c_str());
    }
  }

  void ImGuiViewer::guiNodeContextMenu(const std::string &name,
                                       std::shared_ptr<sg::Node> node)
  {
    if (ImGui::BeginPopupContextItem("item context menu")) {
      char buf[256];
      buf[0]='\0';
      if (ImGui::Button("Add new node..."))
        ImGui::OpenPopup("Add new node...");
      if (ImGui::BeginPopup("Add new node...")) {
        if (ImGui::InputText("node type: ", buf,
                             256, ImGuiInputTextFlags_EnterReturnsTrue)) {
          std::cout << "add node: \"" << buf << "\"\n";
          try {
            static int counter = 0;
            std::stringstream ss;
            ss << "userDefinedNode" << counter++;
            node->add(sg::createNode(ss.str(), buf));
          }
          catch (const std::exception &) {
            std::cerr << "invalid node type: " << buf << std::endl;
          }
        }
        ImGui::EndPopup();
      }
      if (ImGui::Button("Set to new node..."))
        ImGui::OpenPopup("Set to new node...");
      if (ImGui::BeginPopup("Set to new node...")) {
        if (ImGui::InputText("node type: ", buf,
                             256, ImGuiInputTextFlags_EnterReturnsTrue)) {
          std::cout << "set node: \"" << buf << "\"\n";
          try {
            static int counter = 0;
            std::stringstream ss;
            ss << "userDefinedNode" << counter++;
            auto newNode = sg::createNode(ss.str(), buf);
            newNode->setParent(node->parent());
            node->parent().setChild(name, newNode);
          } catch (const std::exception &) {
            std::cerr << "invalid node type: " << buf << std::endl;
          }
        }
        ImGui::EndPopup();
      }
      static ImGuiFs::Dialog importdlg;
      const bool importButtonPressed = ImGui::Button("Import...");
      const char* importpath = importdlg.chooseFileDialog(importButtonPressed);
      if (strlen(importpath) > 0) {
        std::cout << "importing OSPSG file from path: "
                  << importpath << std::endl;
        sg::loadOSPSG(node, std::string(importpath));
      }

      static ImGuiFs::Dialog exportdlg;
      const bool exportButtonPressed = ImGui::Button("Export...");
      const char* exportpath = exportdlg.saveFileDialog(exportButtonPressed);
      if (strlen(exportpath) > 0) {
        // Make sure that the file has the .ospsg suffix
        FileName exportfile = FileName(exportpath).setExt(".ospsg");
        std::cout << "writing OSPSG file to path: " << exportfile << std::endl;
        sg::writeOSPSG(node, exportfile);
      }

      ImGui::EndPopup();
    }
  }

  void ImGuiViewer::guiSGTree(const std::string &name,
                              std::shared_ptr<sg::Node> node)
  {
    int styles = 0;
    if (!node->isValid()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f,0.06f, 0.02f,1.f));
      styles++;
    }

    std::string text;

    std::string nameLower = utility::lowerCase(name);
    std::string nodeNameLower = utility::lowerCase(node->name());

    if (nameLower != nodeNameLower)
      text += name + " -> " + node->name() + " : ";
    else
      text += name + " : ";

    guiSingleNode(text, node);

    if (!node->isValid())
      ImGui::PopStyleColor(styles--);

    if (node->hasChildren()) {
      text += node->type() + "##" + std::to_string(node->uniqueID());
      if (ImGui::TreeNodeEx(text.c_str(),
                            (node->numChildren() > 25) ?
                             0 : ImGuiTreeNodeFlags_DefaultOpen)) {
        guiNodeContextMenu(name, node);

        for(auto child : node->children())
          guiSGTree(child.first, child.second);

        ImGui::TreePop();
      }
    }

    if (ImGui::IsItemHovered() && !node->documentation().empty())
      ImGui::SetTooltip("%s", node->documentation().c_str());
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

} // ::ospray
