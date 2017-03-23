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

#include "imguiViewerSg.h"
#include "common/sg/common/FrameBuffer.h"
#include "transferFunction.h"

#include <imgui.h>
#include <sstream>

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

  ImGuiViewerSg::ImGuiViewerSg(const std::shared_ptr<sg::Node> &scenegraph,
                               const std::shared_ptr<sg::Node> &scenegraphDW
                               )
    : ImGui3DWidget(ImGui3DWidget::FRAMEBUFFER_NONE),
      scenegraph(scenegraph),
      renderEngine(scenegraph, scenegraphDW),scenegraphDW(scenegraphDW)
  {
    setWorldBounds(scenegraph->child("world").bounds());
    renderEngine.setFbSize({1024, 768});

    renderEngine.start();

    originalView = viewPort;
  }

  ImGuiViewerSg::~ImGuiViewerSg()
  {
    renderEngine.stop();
  }

  void ImGuiViewerSg::reshape(const vec2i &newSize)
  {
    ImGui3DWidget::reshape(newSize);
    windowSize = newSize;

    viewPort.modified = true;

    renderEngine.setFbSize(newSize);
    scenegraph->child("frameBuffer")["size"].setValue(newSize);

    pixelBuffer.resize(newSize.x * newSize.y);
  }

  void ImGuiViewerSg::keypress(char key)
  {
    switch (key) {
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

  void ImGuiViewerSg::resetView()
  {
    auto oldAspect = viewPort.aspect;
    viewPort = originalView;
    viewPort.aspect = oldAspect;
  }

  void ImGuiViewerSg::printViewport()
  {
    printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
           viewPort.from.x, viewPort.from.y, viewPort.from.z,
           viewPort.up.x,   viewPort.up.y,   viewPort.up.z,
           viewPort.at.x,   viewPort.at.y,   viewPort.at.z);
    fflush(stdout);
  }

  void ImGuiViewerSg::saveScreenshot(const std::string &basename)
  {
    writePPM(basename + ".ppm", windowSize.x, windowSize.y, pixelBuffer.data());
    std::cout << "saved current frame to '" << basename << ".ppm'" << std::endl;
  }

  void ImGuiViewerSg::toggleRenderingPaused()
  {
    renderingPaused = !renderingPaused;
    renderingPaused ? renderEngine.stop() : renderEngine.start();
  }

  void ImGuiViewerSg::display()
  {
    if (viewPort.modified) {
      auto dir = viewPort.at - viewPort.from;
      dir = normalize(dir);
      auto &camera = scenegraph->child("camera");
      camera["dir"].setValue(dir);
      camera["pos"].setValue(viewPort.from);
      camera["up"].setValue(viewPort.up);

#if 1
      if (scenegraphDW.get()) {
        auto &camera = scenegraphDW->child("camera");
        camera["dir"].setValue(dir);
        camera["pos"].setValue(viewPort.from);
        camera["up"].setValue(viewPort.up);
      }
#endif

      viewPort.modified = false;
    }

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      auto nPixels = windowSize.x * windowSize.y;

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

  void ImGuiViewerSg::buildGui()
  {
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;

    static bool demo_window = false;

    ImGui::Begin("Viewer Controls: press 'g' to show/hide", nullptr, flags);
    ImGui::SetWindowFontScale(0.5f*fontScale);

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("App")) {

        ImGui::Checkbox("Auto-Rotate", &animating);
        bool paused = renderingPaused;
        if (ImGui::Checkbox("Pause Rendering", &paused)) {
          toggleRenderingPaused();
        }
        if (ImGui::MenuItem("Take Screenshot")) saveScreenshot("ospimguiviewer");
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
                                true, false)) {
      ImGui::NewLine();
      ImGui::Text("OSPRay render rate: %.1f FPS", lastFrameFPS);
      ImGui::Text("  Total GUI frame rate: %.1f FPS", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1fms ", lastTotalTime*1000.f);
      ImGui::Text("  GUI time: %.1fms ", lastGUITime*1000.f);
      ImGui::Text("  display pixel time: %.1fms ", lastDisplayTime*1000.f);
      ImGui3DWidget::display();
      ImGui::NewLine();
    }

    static float vec4fv[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
    if (ImGui::CollapsingHeader("SceneGraph", "SceneGraph", true, true))
      buildGUINode(scenegraph, 0);

    ImGui::End();
  }

  void ImGuiViewerSg::buildGUINode(std::shared_ptr<sg::Node> node, int indent)
  {
    int styles=0;
    if (!node->isValid()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImColor(200, 75, 48,255));
      styles++;
    }
    std::string text;
    text += std::string(node->name()+" : ");
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
      ImGui::Text(text.c_str(),"");
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
      ImGui::Text(text.c_str(),"");
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
      ImGui::Text(text.c_str(),"");
      ImGui::SameLine();
      bool val = node->valueAs<bool>();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::Checkbox(text.c_str(), &val)) {
        node->setValue(val);
      }
    } else if (node->type() == "int") {
      ImGui::Text(text.c_str(),"");
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
      ImGui::Text(text.c_str(),"");
      ImGui::SameLine();
      text = "##"+((std::ostringstream&)(std::ostringstream("")
                                         << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::InputText(text.c_str(), buf,
                           value.size()+256,
                           ImGuiInputTextFlags_EnterReturnsTrue))
        node->setValue(std::string(buf));
    } else { // generic holder node
      text+=node->type();
      text += "##"+((std::ostringstream&)(std::ostringstream("")
                                          << node.get())).str(); //TODO: use unique uuid for every node
      if (ImGui::TreeNodeEx(text.c_str(),
                            (indent > 0) ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
        {
          std::string popupName = "Add Node: ##" +
            ((std::ostringstream&)(std::ostringstream("")
                                   << node.get())).str();
          float value = 1.f;
          static bool addChild = true;
          if (ImGui::BeginPopupContextItem("item context menu")) {
            char buf[256];
            buf[0]='\0';
            if (ImGui::InputText("node name: ", buf,
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
          } if (addChild) {
            if (ImGui::BeginPopup(popupName.c_str())) {
              char buf[256];
              buf[0]='\0';
              if (ImGui::InputText("node name: ", buf, 256,
                                   ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::cout << "add node: \"" << buf << "\"\n";
                try {
                  static int counter = 0;
                  std::stringstream ss;
                  ss << "userDefinedNode" << counter++;
                  node->add(sg::createNode(ss.str(), buf));
                } catch (...) {
                  std::cerr << "invalid node type: " << buf << std::endl;
                }
              }
              ImGui::EndPopup();
            }
            else
              addChild = false;
          }

          if (node->type() == "TransferFunction") {
            if (!node->hasChild("transferFunctionWidget")) {
              std::shared_ptr<sg::TransferFunction> tfn =
                std::dynamic_pointer_cast<sg::TransferFunction>(node);

              node->createChildWithValue("transferFunctionWidget",
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
          buildGUINode(child, ++indent);

        ImGui::TreePop();
      }
    }

    if (!node->isValid())
      ImGui::PopStyleColor(styles--);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(node->documentation().c_str());
  }

}// namepace ospray
