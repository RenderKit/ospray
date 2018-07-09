// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <imgui.h>
#include <imconfig.h>

#include "ospcommon/math.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "tfn_lib/tfn_lib.h"
#include "transferFunction.h"

#include "common/sg/common/Data.h"

using namespace ospcommon;

static float lerp(const float a, const float b, const float t)
{
  return (1.0 - t) * a + t * b;
}

namespace ospray {
namespace imgui3D {

TransferFunction::Line::Line() :
  line({vec2f(0, 0),
  vec2f(1, 1)}),
   color(0xffffffff)
{
}

void TransferFunction::Line::movePoint(const float &startX, const vec2f &end)
{
  // Find the closest point to where the user clicked
  auto fnd = std::min_element(line.begin(), line.end(),
      [&startX](const vec2f &a, const vec2f &b){
        return std::abs(startX - a.x) < std::abs(startX - b.x);
      });
  // If there's no nearby point we need to insert a new one
  // TODO: How much fudge to allow for here?
  if (std::abs(startX - fnd->x) >= 0.01){
    std::vector<vec2f>::iterator split = line.begin();
    for (; split != line.end(); ++split){
      if (split->x < startX && startX < (split + 1)->x){
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

void TransferFunction::Line::removePoint(const float &x)
{
  if (line.size() == 2)
    return;

  // See if we have a segment starting near that point
  auto fnd = std::min_element(line.begin(), line.end(),
      [&x](const vec2f &a, const vec2f &b){
      return std::abs(x - a.x) < std::abs(x - b.x);
      });

  // Don't allow erasure of the start and end points of the line
  if (fnd != line.end() && fnd + 1 != line.end() && fnd != line.begin())
    line.erase(fnd);
}

TransferFunction::TransferFunction(std::shared_ptr<sg::TransferFunction> tfn) :
  transferFcn(tfn),
  activeLine(3),
  tfcnSelection(JET),
  customizing(false),
  fcnChanged(true),
  paletteTex(0),
  textBuffer(512, '\0')
{
  // TODO: Use the transfer function passed to use to configure the initial widget lines
  rgbaLines[0].color = 0xff0000ff;
  rgbaLines[1].color = 0xff00ff00;
  rgbaLines[2].color = 0xffff0000;
  rgbaLines[3].color = 0xffffffff;
  setColorMap(false);
}

TransferFunction::~TransferFunction()
{
  if (paletteTex)
    glDeleteTextures(1, &paletteTex);
}

TransferFunction::TransferFunction(const TransferFunction &t) :
  transferFcn(t.transferFcn),
  rgbaLines(t.rgbaLines),
  activeLine(t.activeLine),
  tfcnSelection(t.tfcnSelection),
  customizing(t.customizing),
  transferFunctions(t.transferFunctions),
  fcnChanged(true),
  paletteTex(0),
  textBuffer(512, '\0')
{
  setColorMap(false);
}

TransferFunction& TransferFunction::operator=(const TransferFunction &t)
{
  if (this == &t)
    return *this;

  transferFcn = t.transferFcn;
  rgbaLines = t.rgbaLines;
  activeLine = t.activeLine;
  tfcnSelection = t.tfcnSelection;
  customizing = t.customizing;
  transferFunctions = t.transferFunctions;
  fcnChanged = true;
  setColorMap(false);
  return *this;
}

void TransferFunction::setColorMapByName(std::string name, bool useOpacities)
{
  for (int i =0; i < transferFunctions.size(); i++)
  {
    if (name == transferFunctions[i].name)
    {
      tfcnSelection = i;
      setColorMap(useOpacities);
      return;
    }
  }
}

void TransferFunction::drawUi()
{
    ImGui::Text("Left click and drag to add/move points\nRight click to remove\n");
    ImGui::InputText("filename", textBuffer.data(), textBuffer.size() - 1);

    if (ImGui::Button("Save")){
      save(textBuffer.data());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")){
      load(textBuffer.data());
    }

    std::vector<const char*> colorMaps(transferFunctions.size(), nullptr);
    std::transform(transferFunctions.begin(), transferFunctions.end(), colorMaps.begin(),
        [](const tfn::TransferFunction &t) { return t.name.c_str(); });
    if (ImGui::Combo("ColorMap", &tfcnSelection, colorMaps.data(), colorMaps.size())) {
      setColorMap(false);
    }

    ImGui::Checkbox("Customize", &customizing);
    if (customizing) {
      ImGui::SameLine();
      ImGui::RadioButton("Red", &activeLine, 0); ImGui::SameLine();
      ImGui::SameLine();
      ImGui::RadioButton("Green", &activeLine, 1); ImGui::SameLine();
      ImGui::SameLine();
      ImGui::RadioButton("Blue", &activeLine, 2); ImGui::SameLine();
      ImGui::SameLine();
      ImGui::RadioButton("Alpha", &activeLine, 3);
    } else {
      activeLine = 3;
    }

    vec2f canvasPos(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
    vec2f canvasSize(ImGui::GetContentRegionAvail().x, std::min(ImGui::GetContentRegionAvail().y, 255.0f));
    // Force some min size of the editor
    if (canvasSize.x < 50.f){
      canvasSize.x = 50.f;
    }
    if (canvasSize.y < 50.f){
      canvasSize.y = 50.f;
    }

    if (paletteTex){
      ImGui::Image(reinterpret_cast<void*>(paletteTex), ImVec2(canvasSize.x, 16));
      canvasPos.y += 20;
      canvasSize.y -= 20;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(canvasPos, canvasPos + canvasSize, IM_COL32_WHITE);

    const vec2f viewScale(canvasSize.x, -canvasSize.y + 10);
    const vec2f viewOffset(canvasPos.x, canvasPos.y + canvasSize.y - 10);

    ImGui::InvisibleButton("canvas", canvasSize);
    if (ImGui::IsItemHovered()){
      vec2f mousePos = vec2f(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
      mousePos = (mousePos - viewOffset) / viewScale;
      // Need to somehow find which line of RGBA the mouse is closest too
      if (ImGui::GetIO().MouseDown[0]){
        rgbaLines[activeLine].movePoint(mousePos.x, mousePos);
        fcnChanged = true;
      } else if (ImGui::IsMouseClicked(1)){
        rgbaLines[activeLine].removePoint(mousePos.x);
        fcnChanged = true;
      }
    }
    draw_list->PushClipRect(canvasPos, canvasPos + canvasSize);

    if (customizing) {
      for (int i = 0; i < static_cast<int>(rgbaLines.size()); ++i){
        if (i == activeLine){
          continue;
        }
        for (size_t j = 0; j < rgbaLines[i].line.size() - 1; ++j){
          const vec2f &a = rgbaLines[i].line[j];
          const vec2f &b = rgbaLines[i].line[j + 1];
          draw_list->AddLine(viewOffset + viewScale * a, viewOffset + viewScale * b,
              rgbaLines[i].color, 2.0f);
        }
      }
    }
    // Draw the active line on top
    for (size_t j = 0; j < rgbaLines[activeLine].line.size() - 1; ++j){
      const vec2f &a = rgbaLines[activeLine].line[j];
      const vec2f &b = rgbaLines[activeLine].line[j + 1];
      draw_list->AddLine(viewOffset + viewScale * a, viewOffset + viewScale * b,
          rgbaLines[activeLine].color, 2.0f);
      draw_list->AddCircleFilled(viewOffset + viewScale * a, 4.f, rgbaLines[activeLine].color);
      // If we're last draw the list Circle as well
      if (j == rgbaLines[activeLine].line.size() - 2) {
        draw_list->AddCircleFilled(viewOffset + viewScale * b, 4.f, rgbaLines[activeLine].color);
      }
    }
    draw_list->PopClipRect();
}

void TransferFunction::render()
{
  if (!transferFcn)
    return;
  const int samples = transferFcn->child("numSamples").valueAs<int>();
  // Upload to GL if the transfer function has changed
  if (!paletteTex) {
    GLint prevBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);
    glGenTextures(1, &paletteTex);

    glBindTexture(GL_TEXTURE_2D, paletteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, samples, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (prevBinding)
      glBindTexture(GL_TEXTURE_2D, prevBinding);
  }

  if (fcnChanged) {
    GLint prevBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);

    // Sample the palette then upload the data
    std::vector<uint8_t> palette(samples * 4, 0);

    // Step along the alpha line and sample it
    std::array<std::vector<vec2f>::const_iterator, 4> lit = {
      rgbaLines[0].line.begin(), rgbaLines[1].line.begin(),
      rgbaLines[2].line.begin(), rgbaLines[3].line.begin()
    };
    const float step = 1.0 / samples;

    for (size_t i = 0; i < samples; ++i){
      const float x = step * i;
      std::array<float, 4> sampleColor;
      for (size_t j = 0; j < 4; ++j){
        if (x > (lit[j] + 1)->x) {
          ++lit[j];
        }
        assert(lit[j] != rgbaLines[j].line.end());
        const float t = (x - lit[j]->x) / ((lit[j] + 1)->x - lit[j]->x);
        // It's hard to click down at exactly 0, so offset a little bit
        sampleColor[j] = clamp(lerp(lit[j]->y - 0.001, (lit[j] + 1)->y - 0.001, t));
      }
      for (size_t j = 0; j < 3; ++j) {
        palette[i * 4 + j] = clamp(sampleColor[j] * 255.0, 0.0, 255.0);
      }
      palette[i * 4 + 3] = 255;
    }

    glBindTexture(GL_TEXTURE_2D, paletteTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, samples, 1, GL_RGBA,
                    GL_UNSIGNED_BYTE, static_cast<const void*>(palette.data()));

    auto &colorCP = *transferFcn->child("colorControlPoints").nodeAs<sg::DataVector4f>();
    auto &opacityCP = *transferFcn->child("opacityControlPoints").nodeAs<sg::DataVector2f>();
    opacityCP.clear();
    colorCP.clear();
    for (auto line : rgbaLines[3].line)
      opacityCP.push_back(line);
    int i = 0;
    for (auto line : rgbaLines[0].line) {
      colorCP.push_back(vec4f(line.x, line.y, 0.f, 0.f));
      colorCP.v.back()[2] = rgbaLines[1].line[i].y;
      colorCP.v.back()[3] = rgbaLines[2].line[i].y;
      i++;
    }

    // NOTE(jda) - HACK! colors array isn't updating, so we have to forcefully
    //             say "make sure you update yourself"...???
    opacityCP.markAsModified();
    colorCP.markAsModified();

    // NOTE(jda) - MORE HACKS! (ugh)
    auto &colors    = *transferFcn->child("colors").nodeAs<sg::DataBuffer>();
    auto &opacities = *transferFcn->child("opacities").nodeAs<sg::DataBuffer>();

    colors.markAsModified();
    opacities.markAsModified();

    transferFcn->updateChildDataValues();

    if (prevBinding)
      glBindTexture(GL_TEXTURE_2D, prevBinding);

    fcnChanged = false;
  }
}

void TransferFunction::load(const ospcommon::FileName &fileName)
{
  try {
    tfn::TransferFunction loaded(fileName);
    transferFunctions.emplace_back(fileName);
    tfcnSelection = transferFunctions.size() - 1;
    setColorMap(true);
  } catch (const std::runtime_error &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}

void TransferFunction::save(const ospcommon::FileName &fileName) const
{
  // For opacity we can store the associated data value and only have 1 line,
  // so just save it out directly
  tfn::TransferFunction output(transferFunctions[tfcnSelection].name,
      std::vector<vec3f>(), rgbaLines[3].line, 0, 1, 1);

  // Pull the RGB line values to compute the transfer function and save it out
  // here we may need to do some interpolation, if the RGB lines have differing numbers
  // of control points
  // Find which x values we need to sample to get all the control points for the tfcn.
  std::vector<float> controlPoints;
  for (size_t i = 0; i < 3; ++i) {
    for (const auto &x : rgbaLines[i].line)
      controlPoints.push_back(x.x);
  }

  // Filter out same or within epsilon control points to get unique list
  std::sort(controlPoints.begin(), controlPoints.end());
  auto uniqueEnd = std::unique(controlPoints.begin(), controlPoints.end(),
      [](const float &a, const float &b) { return std::abs(a - b) < 0.0001; });
  controlPoints.erase(uniqueEnd, controlPoints.end());

  // Step along the lines and sample them
  std::array<std::vector<vec2f>::const_iterator, 3> lit = {
    rgbaLines[0].line.begin(), rgbaLines[1].line.begin(),
    rgbaLines[2].line.begin()
  };

  for (const auto &x : controlPoints) {
    std::array<float, 3> sampleColor;
    for (size_t j = 0; j < 3; ++j) {
      if (x > (lit[j] + 1)->x)
        ++lit[j];

      assert(lit[j] != rgbaLines[j].line.end());
      const float t = (x - lit[j]->x) / ((lit[j] + 1)->x - lit[j]->x);
      // It's hard to click down at exactly 0, so offset a little bit
      sampleColor[j] = clamp(lerp(lit[j]->y - 0.001, (lit[j] + 1)->y - 0.001, t));
    }
    output.rgbValues.push_back(vec3f(sampleColor[0], sampleColor[1], sampleColor[2]));
  }

  try {
    output.save(fileName);
  } catch (const std::runtime_error &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}

void TransferFunction::setColorMap(const bool useOpacity)
{
  if (transferFunctions.size() < 1)
    return;
  const auto &colors    = transferFunctions[tfcnSelection].rgbValues;
  const auto &opacities = transferFunctions[tfcnSelection].opacityValues;

  for (size_t i = 0; i < 3; ++i)
    rgbaLines[i].line.clear();

  const float nColors = static_cast<float>(colors.size()) - 1;
  for (size_t i = 0; i < colors.size(); ++i) {
    for (size_t j = 0; j < 3; ++j)
      rgbaLines[j].line.push_back(vec2f(i / nColors, colors[i][j]));
  }

  if (useOpacity && !opacities.empty()) {
    rgbaLines[3].line.clear();
    const double valMin = transferFunctions[tfcnSelection].dataValueMin;
    const double valMax = transferFunctions[tfcnSelection].dataValueMax;
    // Setup opacity if the colormap has it
    for (size_t i = 0; i < opacities.size(); ++i) {
      const float x = (opacities[i].x - valMin) / (valMax - valMin);
      rgbaLines[3].line.push_back(vec2f(x, opacities[i].y));
    }
  }
  fcnChanged = true;
}
void TransferFunction::loadColorMapPresets(std::shared_ptr<sg::Node> tfPresets)
{
  for (auto preset : tfPresets->children())
  {
    std::vector<vec3f> colors;
    std::vector<vec2f> opacities;
    auto& tfPreset = *preset.second->nodeAs<sg::TransferFunction>();
    auto& colorsDV = tfPreset["colors"].nodeAs<sg::DataVector3f>()->v;
    for (auto color : colorsDV)
      colors.push_back(color);
    auto& opacitiesDV = tfPreset["opacityControlPoints"].nodeAs<sg::DataVector2f>()->v;
    for (auto opacity : opacitiesDV)
      opacities.push_back(opacity);
    transferFunctions.emplace_back(tfPreset.name(), colors, opacities, 0, 1, 1);
  }
}

void TransferFunction::loadColorMapPresets()
{
  std::vector<vec3f> colors;
  // The presets have no existing opacity value
  const std::vector<vec2f> opacity;
  // From the old volume viewer, these are based on ParaView
  // Jet transfer function
  colors.push_back(vec3f(0       , 0, 0.562493));
  colors.push_back(vec3f(0       , 0, 1       ));
  colors.push_back(vec3f(0       , 1, 1       ));
  colors.push_back(vec3f(0.500008, 1, 0.500008));
  colors.push_back(vec3f(1       , 1, 0       ));
  colors.push_back(vec3f(1       , 0, 0       ));
  colors.push_back(vec3f(0.500008, 0, 0       ));
  transferFunctions.emplace_back("Jet", colors, opacity, 0, 1, 1);
  colors.clear();

  colors.push_back(vec3f(0        , 0          , 0          ));
  colors.push_back(vec3f(0        , 0.120394   , 0.302678   ));
  colors.push_back(vec3f(0        , 0.216587   , 0.524575   ));
  colors.push_back(vec3f(0.0552529, 0.345022   , 0.659495   ));
  colors.push_back(vec3f(0.128054 , 0.492592   , 0.720287   ));
  colors.push_back(vec3f(0.188952 , 0.641306   , 0.792096   ));
  colors.push_back(vec3f(0.327672 , 0.784939   , 0.873426   ));
  colors.push_back(vec3f(0.60824  , 0.892164   , 0.935546   ));
  colors.push_back(vec3f(0.881376 , 0.912184   , 0.818097   ));
  colors.push_back(vec3f(0.9514   , 0.835615   , 0.449271   ));
  colors.push_back(vec3f(0.904479 , 0.690486   , 0          ));
  colors.push_back(vec3f(0.854063 , 0.510857   , 0          ));
  colors.push_back(vec3f(0.777096 , 0.330175   , 0.000885023));
  colors.push_back(vec3f(0.672862 , 0.139086   , 0.00270085 ));
  colors.push_back(vec3f(0.508812 , 0          , 0          ));
  colors.push_back(vec3f(0.299413 , 0.000366217, 0.000549325));
  colors.push_back(vec3f(0.0157473, 0.00332647 , 0          ));
  transferFunctions.emplace_back("Ice Fire", colors, opacity, 0, 1, 1);
  colors.clear();

  colors.push_back(vec3f(0.231373, 0.298039 , 0.752941));
  colors.push_back(vec3f(0.865003, 0.865003 , 0.865003));
  colors.push_back(vec3f(0.705882, 0.0156863, 0.14902));
  transferFunctions.emplace_back("Cool Warm", colors, opacity, 0, 1, 1);
  colors.clear();

  colors.push_back(vec3f(0, 0, 1));
  colors.push_back(vec3f(1, 0, 0));
  transferFunctions.emplace_back("Blue Red", colors, opacity, 0, 1, 1);
  colors.clear();

  colors.push_back(vec3f(0));
  colors.push_back(vec3f(1));
  transferFunctions.emplace_back("Grayscale", colors, opacity, 0, 1, 1);
  colors.clear();
}

}// ::imgui3D
}// ::ospray

