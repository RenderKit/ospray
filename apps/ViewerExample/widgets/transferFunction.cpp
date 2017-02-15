#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <imgui.h>
#include <imconfig.h>

#include <ospcommon/math.h>
#include <ospray/ospray_cpp/Data.h>
#include "transferFunction.h"

using namespace ospcommon;

static float lerp(const float a, const float b, const float t) {
  return (1.0 - t) * a + t * b;
}

namespace ospray {

TransferFunction::Line::Line() : line({vec2f(0, 0), vec2f(1, 1)}), color(0xffffffff) {}
void TransferFunction::Line::move_point(const float &start_x, const vec2f &end){
  // Find the closest point to where the user clicked
  auto fnd = std::min_element(line.begin(), line.end(),
      [&start_x](const vec2f &a, const vec2f &b){
      return std::abs(start_x - a.x) < std::abs(start_x - b.x);
      });
  // If there's no nearby point we need to insert a new one
  // TODO: How much fudge to allow for here?
  if (std::abs(start_x - fnd->x) >= 0.01){
    std::vector<vec2f>::iterator split = line.begin();
    for (; split != line.end(); ++split){
      if (split->x < start_x && start_x < (split + 1)->x){
        break;
      }
    }
    assert(split != line.end());
    line.insert(split + 1, end);
  } else {
    *fnd = end;
    // Keep the start and end points clamped to the left/right side
    if (fnd == line.begin()){
      fnd->x = 0;
    } else if (fnd == line.end() - 1){
      fnd->x = 1;
    } else {
      // If it's a point in the middle keep it from going past its neighbors
      fnd->x = clamp(fnd->x, (fnd - 1)->x, (fnd + 1)->x);
    }
  }
}
void TransferFunction::Line::remove_point(const float &x){
  if (line.size() == 2){
    return;
  }
  // See if we have a segment starting near that point
  auto fnd = std::min_element(line.begin(), line.end(),
      [&x](const vec2f &a, const vec2f &b){
      return std::abs(x - a.x) < std::abs(x - b.x);
      });
  // Don't allow erasure of the start and end points of the line
  if (fnd != line.end() && fnd + 1 != line.end() && fnd != line.begin()){
    line.erase(fnd);
  }
}

// TODO WILL: The save/load code should use tfn_lib from apps
const int32_t TransferFunction::MAGIC;

TransferFunction::TransferFunction(std::shared_ptr<sg::TransferFunction> &tfn)
  : transferFcn(tfn), active_line(3), fcn_changed(true), palette_tex(0)
{
  // TODO: Use the transfer function passed to use to configure the initial widget lines
  rgba_lines[0].color = 0xff0000ff;
  rgba_lines[1].color = 0xff00ff00;
  rgba_lines[2].color = 0xffff0000;
  rgba_lines[3].color = 0xffffffff;
}
TransferFunction::~TransferFunction(){
  if (palette_tex){
    glDeleteTextures(1, &palette_tex);
  }
}
TransferFunction::TransferFunction(const TransferFunction &t) : transferFcn(t.transferFcn),
  rgba_lines(t.rgba_lines), active_line(t.active_line), fcn_changed(true), palette_tex(0)
{}
TransferFunction& TransferFunction::operator=(const TransferFunction &t) {
  if (this == &t) {
    return *this;
  }
  transferFcn = t.transferFcn;
  rgba_lines = t.rgba_lines;
  active_line = t.active_line;
  fcn_changed = true;
}
void TransferFunction::drawUi(){
  if (ImGui::Begin("Transfer Function")){
    ImGui::Text("Left click and drag to add/move points\nRight click to remove\n");
    /*
    if (ImGui::Button("Save")){
      save_fcn();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")){
      load_fcn();
    }
    */
    ImGui::RadioButton("Red", &active_line, 0); ImGui::SameLine();
    ImGui::RadioButton("Green", &active_line, 1); ImGui::SameLine();
    ImGui::RadioButton("Blue", &active_line, 2); ImGui::SameLine();
    ImGui::RadioButton("Alpha", &active_line, 3);

    vec2f canvas_pos(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
    vec2f canvas_size(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
    // Force some min size of the editor
    if (canvas_size.x < 50.f){
      canvas_size.x = 50.f;
    }
    if (canvas_size.y < 50.f){
      canvas_size.y = 50.f;
    }

    if (palette_tex){
      ImGui::Image(reinterpret_cast<void*>(palette_tex), ImVec2(canvas_size.x, 16));
      canvas_pos.y += 20;
      canvas_size.y -= 20;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, ImColor(255, 255, 255));

    const vec2f view_scale(canvas_size.x, -canvas_size.y);
    const vec2f view_offset(canvas_pos.x, canvas_pos.y + canvas_size.y);

    ImGui::InvisibleButton("canvas", canvas_size);
    if (ImGui::IsItemHovered()){
      vec2f mouse_pos = vec2f(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
      mouse_pos = (mouse_pos - view_offset) / view_scale;
      // Need to somehow find which line of RGBA the mouse is closest too
      if (ImGui::GetIO().MouseDown[0]){
        rgba_lines[active_line].move_point(mouse_pos.x, mouse_pos);
        fcn_changed = true;
      } else if (ImGui::IsMouseClicked(1)){
        rgba_lines[active_line].remove_point(mouse_pos.x);
        fcn_changed = true;
      }
    }
    draw_list->PushClipRect(canvas_pos, canvas_pos + canvas_size);

    // TODO: Should also draw little boxes showing the clickable region for each
    // line segment
    for (int i = 0; i < static_cast<int>(rgba_lines.size()); ++i){
      if (i == active_line){
        continue;
      }
      for (size_t j = 0; j < rgba_lines[i].line.size() - 1; ++j){
        const vec2f &a = rgba_lines[i].line[j];
        const vec2f &b = rgba_lines[i].line[j + 1];
        draw_list->AddLine(view_offset + view_scale * a, view_offset + view_scale * b,
            rgba_lines[i].color, 2.0f);
      }
    }
    // Draw the active line on top
    for (size_t j = 0; j < rgba_lines[active_line].line.size() - 1; ++j){
      const vec2f &a = rgba_lines[active_line].line[j];
      const vec2f &b = rgba_lines[active_line].line[j + 1];
      draw_list->AddLine(view_offset + view_scale * a, view_offset + view_scale * b,
          rgba_lines[active_line].color, 2.0f);
    }
    draw_list->PopClipRect();
  }
  ImGui::End();
}
void TransferFunction::render() {
  // TODO: How many samples for a palette? 128 or 256 is probably plent
  const int samples = 256;
  // Upload to GL if the transfer function has changed
  if (!palette_tex){
    GLint prev_binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);
    glGenTextures(1, &palette_tex);

    glBindTexture(GL_TEXTURE_2D, palette_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, samples, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (prev_binding) {
      glBindTexture(GL_TEXTURE_2D, prev_binding);
    }
  }
  if (fcn_changed){
    GLint prev_binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);

    // Sample the palette then upload the data
    std::vector<uint8_t> palette(samples * 4, 0);
    std::vector<vec3f> ospColors(samples, vec3f(0));
    std::vector<vec2f> ospAlpha(samples, vec2f(0));
    // Step along the alpha line and sample it
    std::array<std::vector<vec2f>::const_iterator, 4> lit = {
      rgba_lines[0].line.begin(), rgba_lines[1].line.begin(),
      rgba_lines[2].line.begin(), rgba_lines[3].line.begin()
    };
    const float step = 1.0 / samples;

    for (size_t i = 0; i < samples; ++i){
      const float x = step * i;
      std::array<float, 4> sampleColor;
      for (size_t j = 0; j < 4; ++j){
        if (x > (lit[j] + 1)->x){
          ++lit[j];
        }
        assert(lit[j] != rgba_lines[j].line.end());
        const float t = (x - lit[j]->x) / ((lit[j] + 1)->x - lit[j]->x);
        // It's hard to click down at exactly 0, so offset a little bit
        sampleColor[j] = clamp(lerp(lit[j]->y - 0.001, (lit[j] + 1)->y - 0.001, t));
      }
      for (size_t j = 0; j < 3; ++j) {
        palette[i * 4 + j] = static_cast<uint8_t>(std::pow(sampleColor[j], 1.0 / 2.2) * 255.0);
        ospColors[i][j] = sampleColor[j];
      }
      ospAlpha[i].x = x;
      ospAlpha[i].y = sampleColor[3];
      palette[i * 4 + 3] = 255;
    }
    glBindTexture(GL_TEXTURE_2D, palette_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, samples, 1, GL_RGBA, GL_UNSIGNED_BYTE,
        static_cast<const void*>(palette.data()));

    transferFcn->setColorMap(ospColors);
    transferFcn->setAlphaMap(ospAlpha);

    if (prev_binding) {
      glBindTexture(GL_TEXTURE_2D, prev_binding);
    }
    fcn_changed = false;
  }
}
/*
bool TransferFunction::load_fcn(const vl::FileName &file_name){
  std::ifstream fin{file_name.c_str(), std::ios::binary};
  int32_t magic = 0;
  if (!fin){
    std::string msg = "Could not open file '" + file_name.file_name + "'";
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "File Not Found", msg.c_str(), NULL);
    return false;
  }
  fin.read(reinterpret_cast<char*>(&magic), sizeof(int32_t));
  if (magic != MAGIC){
    std::string msg = "File '" + file_name.file_name + "' is not a valid TransferFunction";
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid File", msg.c_str(), NULL);
    return false;
  }
  for (auto &line : rgba_lines){
    line.line.clear();
    uint64_t n = 0;
    fin.read(reinterpret_cast<char*>(&n), sizeof(uint64_t));
    line.line.resize(n, vec2f(-1));
    fin.read(reinterpret_cast<char*>(line.line.data()), sizeof(vec2f) * n);
  }
  fcn_changed = true;
  return true;
}
*/
void TransferFunction::save_fcn() const {
  /*
     vl::FileDialog dialog{"vlfn"};
  // The file format is an int32 magic number followed by the red, green, blue and alpha lines
  // Each color line begins with a uint64_t specifying the number of points making
  // the line followed by the vec2 points on the line
  vl::FileResult fresult = dialog.save_file();
  if (fresult.status != vl::FileDialogStatus::FILE_OKAY){
  if (fresult.status == vl::FileDialogStatus::FILE_ERROR){
  std::cout << "Dialog error: " << fresult.result << "\n";
  }
  return;
  }
  if (fresult.result.size() < 6 || fresult.result.extension() != "vlfn"){
  fresult.result.file_name += ".vlfn";
  }
  std::ofstream fout{fresult.result.c_str(), std::ios::binary};
  fout.write(reinterpret_cast<const char*>(&MAGIC), sizeof(int32_t));
  for (const auto &line : rgba_lines){
  const uint64_t n = line.line.size();
  fout.write(reinterpret_cast<const char*>(&n), sizeof(uint64_t));
  fout.write(reinterpret_cast<const char*>(line.line.data()), sizeof(vec2) * n);
  }
  */
}
void TransferFunction::load_fcn(){
  /*
   * TODO: Add native file dialog as well.
   vl::FileDialog dialog{"vlfn"};
   vl::FileResult fresult = dialog.open_file();
   if (fresult.status != vl::FileDialogStatus::FILE_OKAY){
   if (fresult.status == vl::FileDialogStatus::FILE_ERROR){
   std::cout << "Dialog error: " << fresult.result << "\n";
   }
   return;
   }
   load_fcn(fresult.result);
   */
}

}


