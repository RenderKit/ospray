// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2016-2017 Intel Corporation                                    //
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
#include "ospcommon/utility/SaveImage.h"
#include "ospcommon/utility/getEnvVar.h"

#include "imguiViewer.h"

#include "common/sg/common/FrameBuffer.h"
#include "transferFunction.h"

#include <imgui.h>
#include <imguifilesystem/imguifilesystem.h>
#include <sstream>

using std::string;
using namespace ospcommon;

// ImGuiViewer definitions ////////////////////////////////////////////////////

namespace ospray {

  ImGuiViewer::ImGuiViewer(const std::shared_ptr<sg::Node> &scenegraph)
    : ImGuiViewer(scenegraph, nullptr)
  {}

  ImGuiViewer::ImGuiViewer(const std::shared_ptr<sg::Node> &scenegraph,
                           const std::shared_ptr<sg::Node> &scenegraphDW)
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

    //do initial commit to make sure bounds are correctly computed
    scenegraph->traverse("verify");
    scenegraph->traverse("commit");
    auto bbox = scenegraph->child("world").bounds();
    if (bbox.empty()) {
      bbox.lower = vec3f(-5,0,-5);
      bbox.upper = vec3f(5,10,5);
    }
    setWorldBounds(bbox);
    renderEngine.setFbSize({1024, 768});

    renderEngine.start();

    originalView = viewPort;
  }

  ImGuiViewer::~ImGuiViewer()
  {
    renderEngine.stop();
  }

  void ImGuiViewer::mouseButton(int button, int action, int mods)
  {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS
        && (mods & GLFW_MOD_SHIFT))
    {
      const vec2f pos(currMousePos.x / static_cast<float>(windowSize.x),
                      1.f - currMousePos.y / static_cast<float>(windowSize.y));
      renderEngine.pick(pos);
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
      if (scenegraph && scenegraph->hasChild("animationcontroller"))
      {
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
        // No conversion operator or ctor??
        viewPort.at.x = picked.position.x;
        viewPort.at.y = picked.position.y;
        viewPort.at.z = picked.position.z;
        viewPort.modified = true;
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

    static bool demo_window = false;

    ImGui::Begin("Viewer Controls: press 'g' to show/hide", nullptr, flags);
    ImGui::SetWindowFontScale(0.5f*fontScale);

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("App")) {

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

        if (ImGui::Checkbox("Orbit Camera Mode", &orbitMode))
          manipulator = inspectCenterManipulator;

        if (orbitMode) ImGui::Checkbox("Anchor 'Up' Direction", &upAnchored);

        if (ImGui::Checkbox("Fly Camera Mode", &flyMode))
          manipulator = moveModeManipulator;

        if (ImGui::MenuItem("Reset View")) resetView();
        if (ImGui::MenuItem("Reset Accumulation")) viewPort.modified = true;
        if (ImGui::MenuItem("Print View")) printViewport();

        ImGui::EndMenu();
      }

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

      ImGui::EndMenuBar();
    }

    if (demo_window) ImGui::ShowTestWindow(&demo_window);

    if (ImGui::CollapsingHeader("Rendering Statistics", "Rendering Statistics",
                                true, false)) {
      ImGui::NewLine();
      ImGui::Text("OSPRay render rate: %.1f fps", lastFrameFPS);
      ImGui::Text("  Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1f ms", lastTotalTime*1000.f);
      ImGui::Text("  GUI time: %.1f ms", lastGUITime*1000.f);
      ImGui::Text("  display pixel time: %.1f ms", lastDisplayTime*1000.f);
      ImGui::Text("Variance: %.3f", renderEngine.getLastVariance());
      ImGui3DWidget::display();
      ImGui::NewLine();
    }

    if (ImGui::CollapsingHeader("SceneGraph", "SceneGraph", true, true))
      buildGUINode("root", scenegraph, 0);

    ImGui::End();
  }

  void ImGuiViewer::buildGUINode(std::string name,
                                 std::shared_ptr<sg::Node> node,
                                 int indent)
  {
    int styles=0;
    if (!node->isValid()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImColor(200, 75, 48,255));
      styles++;
    }
    std::string text("");
    std::string nameLower=name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    std::string nodeNameLower=node->name();
    std::transform(nodeNameLower.begin(), nodeNameLower.end(), nodeNameLower.begin(), ::tolower);
    if (nameLower != nodeNameLower)
      text += std::string(name+" -> "+node->name()+" : ");
    else
      text += std::string(name+" : ");
    const int numChildren = node->numChildren();
    if (node->type() == "vec3f") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      vec3f val = node->valueAs<vec3f>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if ((node->flags() & sg::NodeFlags::gui_color)) {
        if (ImGui::ColorEdit3(text.c_str(), (float*)&val.x))
          node->setValue(val);
      }
      else if ((node->flags() & sg::NodeFlags::gui_slider)) {
        if (ImGui::SliderFloat3(text.c_str(), &val.x,
                                node->min().get<vec3f>().x,
                                node->max().get<vec3f>().x))
          node->setValue(val);
      }
      else if (ImGui::DragFloat3(text.c_str(), (float*)&val.x, .01f)) {
        node->setValue(val);
      }
    } else if (node->type() == "vec2f") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      vec2f val = node->valueAs<vec2f>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::DragFloat2(text.c_str(), (float*)&val.x, .01f)) {
        node->setValue(val);
      }
    } else if (node->type() == "vec2i") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      vec2i val = node->valueAs<vec2i>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::DragInt2(text.c_str(), (int*)&val.x)) {
        node->setValue(val);
      }
    } else if (node->type() == "float") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      float val = node->valueAs<float>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if ((node->flags() & sg::NodeFlags::gui_slider)) {
        if (ImGui::SliderFloat(text.c_str(), &val,
                               node->min().get<float>(),
                               node->max().get<float>()))
          node->setValue(val);
      }
      else if (ImGui::DragFloat(text.c_str(), &val, .01f)) {
        node->setValue(val);
      }
    } else if (node->type() == "bool") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      bool val = node->valueAs<bool>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::Checkbox(text.c_str(), &val)) {
        node->setValue(val);
      }
    } else if (node->type() == "int") {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      int val = node->valueAs<int>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if ((node->flags() & sg::NodeFlags::gui_slider)) {
        if (ImGui::SliderInt(text.c_str(), &val,
                             node->min().get<int>(),
                             node->max().get<int>()))
          node->setValue(val);
      }
      else if (ImGui::DragInt(text.c_str(), &val)) {
        node->setValue(val);
      }
    } else if (node->type() == "string") {
      std::string value = node->valueAs<std::string>().c_str();
      char* buf = (char*)malloc(value.size()+1+256);
      strcpy(buf,value.c_str());
      buf[value.size()] = '\0';
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::InputText(text.c_str(), buf,
                           value.size()+256,
                           ImGuiInputTextFlags_EnterReturnsTrue))
      {
        node->setValue(std::string(buf));
      }
      free(buf);
    } else if (numChildren == 0) {
      text += node->type();
      ImGui::Text(text.c_str());
    }

    if (numChildren > 0) {
      text+=node->type();
      text += "##"+((std::ostringstream&)(std::ostringstream("")
                                          << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::TreeNodeEx(text.c_str(),
                            (indent > 1 && numChildren > 20) ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
        {
          if (ImGui::BeginPopupContextItem("item context menu")) {
            char buf[256];
            buf[0]='\0';
            static std::shared_ptr<sg::Node> copiedLink = nullptr;
            if (ImGui::Button("CopyLink"))
              copiedLink = node;
            if (ImGui::Button("PasteLink"))
            {
              if (copiedLink)
              {
                copiedLink->setParent(node->parent());
                node->parent().setChild(name, copiedLink);
              }
            }
            if (ImGui::Button("Add new node..."))
              ImGui::OpenPopup("Add new node...");
            if (ImGui::BeginPopup("Add new node..."))
            {
              if (ImGui::InputText("node type: ", buf,
                                   256, ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::cout << "add node: \"" << buf << "\"\n";
                try {
                  static int counter = 0;
                  std::stringstream ss;
                  ss << "userDefinedNode" << counter++;
                  node->add(sg::createNode(ss.str(), buf));
                }
                catch (...)
                {
                  std::cerr << "invalid node type: " << buf << std::endl;
                }
              }
              ImGui::EndPopup();
            }
            if (ImGui::Button("Set to new node..."))
              ImGui::OpenPopup("Set to new node...");
            if (ImGui::BeginPopup("Set to new node..."))
            {
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
                }
                catch (...)
                {
                  std::cerr << "invalid node type: " << buf << std::endl;
                }
              }
              ImGui::EndPopup();
            }
            static ImGuiFs::Dialog importdlg;
            const bool importButtonPressed = ImGui::Button("Import...");
            const char* importpath = importdlg.chooseFileDialog(importButtonPressed);
            if (strlen(importpath) > 0)
            {
              std::cout << "importing OSPSG file from path: " << importpath << std::endl;
              sg::loadOSPSG(node, std::string(importpath));
            }

            static ImGuiFs::Dialog exportdlg;
            const bool exportButtonPressed = ImGui::Button("Export...");
            const char* exportpath = exportdlg.saveFileDialog(exportButtonPressed);
            if (strlen(exportpath) > 0)
            {
              std::cout << "writing OSPSG file to path: " << exportpath << std::endl;
              sg::writeOSPSG(node, std::string(exportpath));
            }

            ImGui::EndPopup();
          }

          if (node->type() == "TransferFunction") {
            if (!node->hasChild("transferFunctionWidget")) {
              std::shared_ptr<sg::TransferFunction> tfn =
                std::dynamic_pointer_cast<sg::TransferFunction>(node);

              node->createChildWithValue("transferFunctionWidget","Node",
                                         TransferFunction(tfn));
            }

            auto &tfnWidget =
              node->child("transferFunctionWidget").valueAs<TransferFunction>();

            tfnWidget.render();
            tfnWidget.drawUi();
          }
        }

        if (!node->isValid())
          ImGui::PopStyleColor(styles--);

        for(auto child : node->children())
          buildGUINode(child.first, child.second, ++indent);

        ImGui::TreePop();
      }
    } else { // generic holder node
    }

    if (!node->isValid())
      ImGui::PopStyleColor(styles--);
    if (ImGui::IsItemHovered())
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

}// ::ospray
