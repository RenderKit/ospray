#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <imgui.h>
#include <imconfig.h>

#include "common/math.h"
#include "TransferFunctionWidget.h"

using namespace tfn;
using namespace tfn_widget;

template<class T>
const T &clamp(const T &v, const T &lo, const T &hi) {
  return (v < lo) ? lo : (hi < v ? hi : v);
}

template<typename T>
int find_idx(const T &A, float p, int l = -1, int r = -1) {
  l = l == -1 ? 0 : l;
  r = r == -1 ? A.size() - 1 : r;
  int m = (r + l) / 2;
  if (A[l].p > p) { return l; }
  else if (A[r].p <= p) { return r + 1; }
  else if ((m == l) || (m == r)) { return m + 1; }
  else {
    if (A[m].p <= p) { return find_idx(A, p, m, r); }
    else { return find_idx(A, p, l, m); }
  }
}

float lerp(const float &l, const float &r,
           const float &pl, const float &pr, const float &p) {
  const float dl = std::abs(pr - pl) > 0.0001f ? (p - pl) / (pr - pl) : 0.f;
  const float dr = 1.f - dl;
  return l * dr + r * dl;
}

void TransferFunctionWidget::LoadDefaultMap() {
  tfn_c_list.emplace_back(4);
  tfn_c_list.back()[0] = ColorPoint(0.0f, 0.f, 0.f, 1.f);
  tfn_c_list.back()[1] = ColorPoint(0.3f, 0.f, 1.f, 1.f);
  tfn_c_list.back()[2] = ColorPoint(0.6f, 1.f, 1.f, 0.f);
  tfn_c_list.back()[3] = ColorPoint(1.0f, 1.f, 0.f, 0.f);
  tfn_o_list.emplace_back(5);
  tfn_o_list.back()[0] = OpacityPoint(0.00f, 0.00f);
  tfn_o_list.back()[1] = OpacityPoint(0.25f, 0.25f);
  tfn_o_list.back()[2] = OpacityPoint(0.50f, 0.50f);
  tfn_o_list.back()[3] = OpacityPoint(0.75f, 0.75f);
  tfn_o_list.back()[4] = OpacityPoint(1.00f, 1.00f);
  tfn_editable.push_back(true);
  tfn_names.push_back("Jet");
};

void
TransferFunctionWidget::SetTFNSelection(int selection) {
  if (tfn_selection != selection) {
    tfn_selection = selection;
    // Remember to update other constructors as well
    tfn_c = &(tfn_c_list[selection]);
    tfn_o = &(tfn_o_list[selection]);
    tfn_edit = tfn_editable[selection];
    tfn_changed = true;
  }
}

TransferFunctionWidget::~TransferFunctionWidget() {
  if (tfn_palette) { glDeleteTextures(1, &tfn_palette); }
}

TransferFunctionWidget::TransferFunctionWidget
  (const std::function<void(const std::vector<float> &,
                            const std::vector<float> &,
			    const std::array<float, 2>&)> &
   sample_value_setter)
  :
  tfn_selection(JET),
  tfn_changed(true),
  tfn_palette(0),
  tfn_text_buffer(512, '\0'),
  tfn_sample_set(sample_value_setter),
  valueRange{0.f,0.f},
  defaultRange{0.f,0.f}
{
  LoadDefaultMap();
  tfn_c = &(tfn_c_list[tfn_selection]);
  tfn_o = &(tfn_o_list[tfn_selection]);
  tfn_edit = tfn_editable[tfn_selection];
}

TransferFunctionWidget::TransferFunctionWidget(const TransferFunctionWidget &core) :
  tfn_c_list(core.tfn_c_list),
  tfn_o_list(core.tfn_o_list),
  tfn_readers(core.tfn_readers),
  tfn_selection(core.tfn_selection),
  tfn_changed(true),
  tfn_palette(0),
  tfn_text_buffer(512, '\0'),
  tfn_sample_set(core.tfn_sample_set),
  valueRange(core.valueRange),
  defaultRange(core.defaultRange)
{
  tfn_c = &(tfn_c_list[tfn_selection]);
  tfn_o = &(tfn_o_list[tfn_selection]);
  tfn_edit = tfn_editable[tfn_selection];
}

TransferFunctionWidget &
TransferFunctionWidget::operator=(const tfn::tfn_widget::TransferFunctionWidget &core) {
  if (this == &core) { return *this; }
  tfn_c_list = core.tfn_c_list;
  tfn_o_list = core.tfn_o_list;
  tfn_readers = core.tfn_readers;
  tfn_selection = core.tfn_selection;
  tfn_changed = true;
  tfn_palette = 0;
  tfn_sample_set = core.tfn_sample_set;
  valueRange[0] = core.valueRange[0];
  valueRange[1] = core.valueRange[1];
  defaultRange[0] = core.defaultRange[0];
  defaultRange[1] = core.defaultRange[1];
  return *this;
}

bool TransferFunctionWidget::drawUI() {
  if (!ImGui::Begin("Transfer Function Widget")) {
    ImGui::End();
    return false;
  }
  ImGui::Text("1D Transfer Function");
  // radio paremeters
  ImGui::Separator();
  std::vector<const char *> names(tfn_names.size(), nullptr);
  std::transform(tfn_names.begin(), tfn_names.end(), names.begin(),
                 [](const std::string &t) { return t.c_str(); });
  ImGui::ListBox("Color maps", &tfn_selection, names.data(), names.size());
  ImGui::InputText("", tfn_text_buffer.data(), tfn_text_buffer.size() - 1);
  ImGui::SameLine();
  if (ImGui::Button("load new file")) {
    try {
      std::string s = tfn_text_buffer.data();
      s.erase(s.find_last_not_of(" \n\r\t") + 1);
      s.erase(0, s.find_first_not_of(" \n\r\t"));
      load(s.c_str());
    } catch (const std::runtime_error &error) {
      std::cerr
        << "\033[1;33m"
        << "Error:" << error.what()
        << "\033[0m"
        << std::endl;
    }
  }
  // TODO: save function is not implemented
  // if (ImGui::Button("save")) { save(tfn_text_buffer.data()); }
  if (ImGui::DragFloat2("value range", valueRange.data(),
			(defaultRange[1] - defaultRange[0]) / 1000.f,
                        defaultRange[0], defaultRange[1])) {
    tfn_changed = true;
  }
  //------------ Transfer Function -------------------
  // style
  // only God and me know what do they do ...
  SetTFNSelection(tfn_selection);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  float canvas_x = ImGui::GetCursorScreenPos().x;
  float canvas_y = ImGui::GetCursorScreenPos().y;
  float canvas_avail_x = ImGui::GetContentRegionAvail().x;
  float canvas_avail_y = ImGui::GetContentRegionAvail().y;
  const float mouse_x = ImGui::GetMousePos().x;
  const float mouse_y = ImGui::GetMousePos().y;
  const float scroll_x = ImGui::GetScrollX();
  const float scroll_y = ImGui::GetScrollY();
  const float margin = 10.f;
  const float width = canvas_avail_x - 2.f * margin;
  const float height = 60.f;
  const float color_len = 9.f;
  const float opacity_len = 7.f;
  bool display_helper_0 = false; // How to operate/delete a control point
  bool display_helper_1 = false; // How to add control points
  // draw preview texture
  ImGui::SetCursorScreenPos(ImVec2(canvas_x + margin, canvas_y));
  ImGui::Image(reinterpret_cast<void *>(tfn_palette), ImVec2(width, height));
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  for (int i = 0; i < tfn_o->size() - 1; ++i) {
    std::vector<ImVec2> polyline;
    polyline.emplace_back(canvas_x + margin + (*tfn_o)[i].p * width,
                          canvas_y + height);
    polyline.emplace_back(canvas_x + margin + (*tfn_o)[i].p * width,
                          canvas_y + height - (*tfn_o)[i].a * height);
    polyline.emplace_back(canvas_x + margin + (*tfn_o)[i + 1].p * width + 1,
                          canvas_y + height - (*tfn_o)[i + 1].a * height);
    polyline.emplace_back(canvas_x + margin + (*tfn_o)[i + 1].p * width + 1,
                          canvas_y + height);
    draw_list->AddConvexPolyFilled(polyline.data(), polyline.size(), 0xFFD8D8D8, true);
  }
  canvas_y += height + margin;
  canvas_avail_y -= height + margin;
  // draw color control points
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  if (tfn_edit) {
    // draw circle background
    draw_list->AddRectFilled(ImVec2(canvas_x + margin, canvas_y - margin),
                             ImVec2(canvas_x + margin + width,
                                    canvas_y - margin + 2.5 * color_len), 0xFF474646);
    // draw circles
    for (int i = tfn_c->size() - 1; i >= 0; --i) {
      const ImVec2 pos(canvas_x + width * (*tfn_c)[i].p + margin, canvas_y);
      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
      // white background
      draw_list->AddTriangleFilled(ImVec2(pos.x - 0.5f * color_len, pos.y),
                                   ImVec2(pos.x + 0.5f * color_len, pos.y),
                                   ImVec2(pos.x, pos.y - color_len),
                                   0xFFD8D8D8);
      draw_list->AddCircleFilled(ImVec2(pos.x, pos.y + 0.5f * color_len), color_len,
                                 0xFFD8D8D8);
      // draw picker
      ImVec4 picked_color = ImColor((*tfn_c)[i].r, (*tfn_c)[i].g, (*tfn_c)[i].b, 1.f);
      ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y + 1.5f * color_len));
      if (ImGui::ColorEdit4(("##ColorPicker" + std::to_string(i)).c_str(),
                            (float *) &picked_color,
                            ImGuiColorEditFlags_NoAlpha |
                              ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_AlphaPreview |
                              ImGuiColorEditFlags_NoOptions |
                              ImGuiColorEditFlags_NoTooltip)) {
        (*tfn_c)[i].r = picked_color.x;
        (*tfn_c)[i].g = picked_color.y;
        (*tfn_c)[i].b = picked_color.z;
        tfn_changed = true;
      }
      if (ImGui::IsItemHovered()) {
        // convert float color to char
        int cr = static_cast<int>(picked_color.x * 255);
        int cg = static_cast<int>(picked_color.y * 255);
        int cb = static_cast<int>(picked_color.z * 255);
        // setup tooltip
        ImGui::BeginTooltip();
        ImVec2 sz(ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.y * 2,
                  ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.y * 2);
        ImGui::ColorButton("##PreviewColor", picked_color,
                           ImGuiColorEditFlags_NoAlpha |
                             ImGuiColorEditFlags_AlphaPreview,
                           sz);
        ImGui::SameLine();
        ImGui::Text("Left click to edit\n"
                      "HEX: #%02X%02X%02X\n"
                      "RGB: [%3d,%3d,%3d]\n(%.2f, %.2f, %.2f)",
                    cr, cg, cb, cr, cg, cb, picked_color.x, picked_color.y, picked_color.z);
        ImGui::EndTooltip();
      }
    }
    for (int i = 0; i < tfn_c->size(); ++i) {
      const ImVec2 pos(canvas_x + width * (*tfn_c)[i].p + margin, canvas_y);
      // draw button
      ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y - 0.5 * color_len));
      ImGui::InvisibleButton(("##ColorControl-" + std::to_string(i)).c_str(),
                             ImVec2(2.f * color_len, 2.f * color_len));
      // dark highlight
      ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y));
      draw_list->AddCircleFilled(ImVec2(pos.x, pos.y + 0.5f * color_len), 0.5f * color_len,
                                 ImGui::IsItemHovered() ? 0xFF051C33 : 0xFFBCBCBC);
      // delete color point
      if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
        if (i > 0 && i < tfn_c->size() - 1) {
          tfn_c->erase(tfn_c->begin() + i);
          tfn_changed = true;
        }
      }
        // drag color control point
      else if (ImGui::IsItemActive()) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        if (i > 0 && i < tfn_c->size() - 1) {
          (*tfn_c)[i].p += delta.x / width;
          (*tfn_c)[i].p = clamp((*tfn_c)[i].p, (*tfn_c)[i - 1].p, (*tfn_c)[i + 1].p);
        }
        tfn_changed = true;
      } else if (ImGui::IsItemHovered()) { display_helper_0 = true; }
    }
  }
  // draw opacity control points
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  {
    // draw circles
    for (int i = 0; i < tfn_o->size(); ++i) {
      const ImVec2 pos(canvas_x + width * (*tfn_o)[i].p + margin,
                       canvas_y - height * (*tfn_o)[i].a - margin);
      ImGui::SetCursorScreenPos(ImVec2(pos.x - opacity_len, pos.y - opacity_len));
      ImGui::InvisibleButton(("##OpacityControl-" + std::to_string(i)).c_str(),
                             ImVec2(2.f * opacity_len, 2.f * opacity_len));
      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
      // dark bounding box
      draw_list->AddCircleFilled(pos, opacity_len, 0xFF565656);
      // white background
      draw_list->AddCircleFilled(pos, 0.8f * opacity_len, 0xFFD8D8D8);
      // highlight
      draw_list->AddCircleFilled(pos, 0.6f * opacity_len,
                                 ImGui::IsItemHovered() ? 0xFF051c33 : 0xFFD8D8D8);
      // setup interaction
      // delete opacity point
      if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
        if (i > 0 && i < tfn_o->size() - 1) {
          tfn_o->erase(tfn_o->begin() + i);
          tfn_changed = true;
        }
      } else if (ImGui::IsItemActive()) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        (*tfn_o)[i].a -= delta.y / height;
        (*tfn_o)[i].a = clamp((*tfn_o)[i].a, 0.0f, 1.0f);
        if (i > 0 && i < tfn_o->size() - 1) {
          (*tfn_o)[i].p += delta.x / width;
          (*tfn_o)[i].p = clamp((*tfn_o)[i].p, (*tfn_o)[i - 1].p, (*tfn_o)[i + 1].p);
        }
        tfn_changed = true;
      } else if (ImGui::IsItemHovered()) { display_helper_0 = true; }
    }
  }
  // draw background interaction
  ImGui::SetCursorScreenPos(ImVec2(canvas_x + margin, canvas_y - margin));
  ImGui::InvisibleButton("##tfn_palette_color", ImVec2(width, 2.5 * color_len));
  // add color point
  if (tfn_edit && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
    const float p = clamp((mouse_x - canvas_x - margin - scroll_x) / (float) width,
                          0.f, 1.f);
    const int ir = find_idx(*tfn_c, p);
    const int il = ir - 1;
    const float pr = (*tfn_c)[ir].p;
    const float pl = (*tfn_c)[il].p;
    const float r = lerp((*tfn_c)[il].r, (*tfn_c)[ir].r, pl, pr, p);
    const float g = lerp((*tfn_c)[il].g, (*tfn_c)[ir].g, pl, pr, p);
    const float b = lerp((*tfn_c)[il].b, (*tfn_c)[ir].b, pl, pr, p);
    ColorPoint pt;
    pt.p = p, pt.r = r;
    pt.g = g;
    pt.b = b;
    tfn_c->insert(tfn_c->begin() + ir, pt);
    tfn_changed = true;
  }
  if (ImGui::IsItemHovered()) { display_helper_1 = true; }
  // draw background interaction
  ImGui::SetCursorScreenPos(ImVec2(canvas_x + margin, canvas_y - height - margin));
  ImGui::InvisibleButton("##tfn_palette_opacity", ImVec2(width, height));
  // add opacity point
  if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
    const float x = clamp((mouse_x - canvas_x - margin - scroll_x) / (float) width,
                          0.f, 1.f);
    const float y = clamp(-(mouse_y - canvas_y + margin - scroll_y) / (float) height,
                          0.f, 1.f);
    const int idx = find_idx(*tfn_o, x);
    OpacityPoint pt;
    pt.p = x, pt.a = y;
    tfn_o->insert(tfn_o->begin() + idx, pt);
    tfn_changed = true;
  }
  if (ImGui::IsItemHovered()) { display_helper_1 = true; }
  // update cursors
  canvas_y += 4.f * color_len + margin;
  canvas_avail_y -= 4.f * color_len + margin;
  //------------ Transfer Function -------------------
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  if (display_helper_0) {
    ImGui::Text("Double right click botton to delete point\n"
                  "Left click and drag to move point");
  }
  if (display_helper_1 && !display_helper_0) {
    ImGui::Text("Double left click empty area to add point\n");
  }
  ImGui::End();
  return true;
}

void RenderTFNTexture(GLuint &tex, int width, int height) {
  GLint prevBinding = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (prevBinding) {
    glBindTexture(GL_TEXTURE_2D, prevBinding);
  }
}

void TransferFunctionWidget::render(size_t tfn_w, size_t tfn_h) {

  // Upload to GL if the transfer function has changed
  if (!tfn_palette) {
    RenderTFNTexture(tfn_palette, tfn_w, tfn_h);
  }

  // Update texture color
  if (tfn_changed) {
    // Backup old states
    GLint prevBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);

    // Sample the palette then upload the data
    std::vector<uint8_t> palette(tfn_w * tfn_h * 4, 0);
    std::vector<float> colors(3 * tfn_w, 1.f);
    std::vector<float> alpha(2 * tfn_w, 1.f);
    const float step = 1.0f / (float) (tfn_w - 1);
    for (int i = 0; i < tfn_w; ++i) {
      const float p = clamp(i * step, 0.0f, 1.0f);
      int ir, il;
      float pr, pl;
      // color
      ir = find_idx(*tfn_c, p);
      il = ir - 1;
      pr = (*tfn_c)[ir].p;
      pl = (*tfn_c)[il].p;
      const float r = lerp((*tfn_c)[il].r, (*tfn_c)[ir].r, pl, pr, p);
      const float g = lerp((*tfn_c)[il].g, (*tfn_c)[ir].g, pl, pr, p);
      const float b = lerp((*tfn_c)[il].b, (*tfn_c)[ir].b, pl, pr, p);
      colors[3 * i + 0] = r;
      colors[3 * i + 1] = g;
      colors[3 * i + 2] = b;
      // opacity
      ir = find_idx(*tfn_o, p);
      il = ir - 1;
      pr = (*tfn_o)[ir].p;
      pl = (*tfn_o)[il].p;
      const float a = lerp((*tfn_o)[il].a, (*tfn_o)[ir].a, pl, pr, p);
      alpha[2 * i + 0] = p;
      alpha[2 * i + 1] = a;
      // palette
      palette[i * 4 + 0] = static_cast<uint8_t>(r * 255.f);
      palette[i * 4 + 1] = static_cast<uint8_t>(g * 255.f);
      palette[i * 4 + 2] = static_cast<uint8_t>(b * 255.f);
      palette[i * 4 + 3] = 255;
    }

    // Render palette again
    glBindTexture(GL_TEXTURE_2D, tfn_palette);
    // LOGICALLY we need to resize texture of texture is resized
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tfn_w, tfn_h,
		 0, GL_RGBA, GL_UNSIGNED_BYTE,
		 static_cast<const void *>(palette.data()));  
    // Restore previous binded texture
    if (prevBinding) { glBindTexture(GL_TEXTURE_2D, prevBinding); }
    tfn_sample_set(colors, alpha, valueRange);
    tfn_changed = false;
  }
}

void TransferFunctionWidget::load(const std::string &fileName) {
  tfn::TransferFunction loaded(fileName);
  tfn_readers.emplace_back(fileName);
  const auto tfn_new = tfn_readers.back();
  const int c_size = tfn_new.rgbValues.size();
  const int o_size = tfn_new.opacityValues.size();
  // load data
  tfn_c_list.emplace_back(c_size);
  tfn_o_list.emplace_back(o_size);
  tfn_editable.push_back(false); // TODO we dont want to edit loaded TFN
  tfn_names.push_back(tfn_new.name);
  SetTFNSelection(tfn_names.size() - 1); // set the loaded function as current
  if (c_size < 2) {
    throw std::runtime_error("transfer function contains too few color points");
  }
  const float c_step = 1.f / (c_size - 1);
  for (int i = 0; i < c_size; ++i) {
    const float p = static_cast<float>(i) * c_step;
    (*tfn_c)[i].p = p;
    (*tfn_c)[i].r = tfn_new.rgbValues[i].x;
    (*tfn_c)[i].g = tfn_new.rgbValues[i].y;
    (*tfn_c)[i].b = tfn_new.rgbValues[i].z;
  }
  if (o_size < 2) {
    throw std::runtime_error("transfer function contains too few opacity points");
  }
  for (int i = 0; i < o_size; ++i) {
    (*tfn_o)[i].p = tfn_new.opacityValues[i].x;
    (*tfn_o)[i].a = tfn_new.opacityValues[i].y;
  }
  tfn_changed = true;
}

void tfn::tfn_widget::TransferFunctionWidget::save(const std::string &fileName)
const
{
  // // For opacity we can store the associated data value and only have 1 line,
  // // so just save it out directly
  // tfn::TransferFunction output(transferFunctions[tfcnSelection].name,
  //     std::vector<vec3f>(), rgbaLines[3].line, 0, 1, 1);

  // // Pull the RGB line values to compute the transfer function and save it out
  // // here we may need to do some interpolation, if the RGB lines have differing numbers
  // // of control points
  // // Find which x values we need to sample to get all the control points for the tfcn.
  // std::vector<float> controlPoints;
  // for (size_t i = 0; i < 3; ++i) {
  //   for (const auto &x : rgbaLines[i].line)
  //     controlPoints.push_back(x.x);
  // }

  // // Filter out same or within epsilon control points to get unique list
  // std::sort(controlPoints.begin(), controlPoints.end());
  // auto uniqueEnd = std::unique(controlPoints.begin(), controlPoints.end(),
  //     [](const float &a, const float &b) { return std::abs(a - b) < 0.0001; });
  // controlPoints.erase(uniqueEnd, controlPoints.end());

  // // Step along the lines and sample them
  // std::array<std::vector<vec2f>::const_iterator, 3> lit = {
  //   rgbaLines[0].line.begin(), rgbaLines[1].line.begin(),
  //   rgbaLines[2].line.begin()
  // };

  // for (const auto &x : controlPoints) {
  //   std::array<float, 3> sampleColor;
  //   for (size_t j = 0; j < 3; ++j) {
  //     if (x > (lit[j] + 1)->x)
  //       ++lit[j];

  //     assert(lit[j] != rgbaLines[j].line.end());
  //     const float t = (x - lit[j]->x) / ((lit[j] + 1)->x - lit[j]->x);
  //     // It's hard to click down at exactly 0, so offset a little bit
  //     sampleColor[j] = clamp(lerp(lit[j]->y - 0.001, (lit[j] + 1)->y - 0.001, t));
  //   }
  //   output.rgbValues.push_back(vec3f(sampleColor[0], sampleColor[1], sampleColor[2]));
  // }

  // output.save(fileName);
;
}
