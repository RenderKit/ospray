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

#pragma once

#include <memory>
#include <vector>
#include <array>
#include <ospcommon/vec.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <tfn_lib/tfn_lib.h>
#include "Imgui3dExport.h"
#include <ospray/ospray_cpp/TransferFunction.h>
#include "common/sg/transferFunction/TransferFunction.h"

namespace ospray {
  namespace imgui3D {

  class OSPRAY_IMGUI3D_INTERFACE TransferFunction
  {
  public:

    TransferFunction(std::shared_ptr<sg::TransferFunction> tfn = nullptr);
    ~TransferFunction();
    TransferFunction(const TransferFunction &t);
    TransferFunction& operator=(const TransferFunction &t);

    void setSGTF(std::shared_ptr<sg::TransferFunction> tf) {
      if (tf != transferFcn) {
        fcnChanged = true;
        transferFcn = tf;
      }
    }

    void loadColorMapPresets(std::shared_ptr<sg::Node> tfPresets);
    /* Draw the transfer function editor widget, returns true if the
     * transfer function changed
    */
    void drawUi();
    /* Render the transfer function to a 1D texture that can
     * be applied to volume data
     */
    void render();
    // Load the transfer function in the file passed and set it active
    void load(const ospcommon::FileName &fileName);
    // Save the current transfer function out to the file
    void save(const ospcommon::FileName &fileName) const;

    void setColorMapByName(std::string name, bool useOpacities = false);

    struct Line
    {
      // A line is made up of points sorted by x, its coordinates are
      // on the range [0, 1]
      std::vector<ospcommon::vec2f> line;
      int color;

      // TODO: Constructor that takes an existing line
      // Construct a new diagonal line: [(0, 0), (1, 1)]
      Line();
      /* Move a point on the line from start to end, if the line is
       * not split at 'start' it will be split then moved
       * TODO: Should we have some cap on the number of points? We should
       * also track if you're actively dragging a point so we don't recreate
       * points if you move the mouse too fast
       */
      void movePoint(const float &startX, const ospcommon::vec2f &end);
      // Remove a point from the line, merging the two segments on either side
      void removePoint(const float &x);
    };

  private:

    // The indices of the transfer function color presets available
    enum ColorMap {
      JET,
      ICE_FIRE,
      COOL_WARM,
      BLUE_RED,
      GRAYSCALE,
    };

    // The scenegraph transfer function being manipulated by this widget
    std::shared_ptr<sg::TransferFunction> transferFcn;
    // Lines for RGBA transfer function controls
    std::array<Line, 4> rgbaLines;
    // The line currently being edited
    int activeLine;
    // The selected transfer function being shown
    int tfcnSelection;
    // If we're customizing the transfer function's RGB values
    bool customizing;
    // The list of avaliable transfer functions, both built-in and loaded
    std::vector<tfn::TransferFunction> transferFunctions;
    // The filename input text buffer
    std::vector<char> textBuffer;

    // Track if the function changed and must be re-uploaded.
    // We start by marking it changed to upload the initial palette
    bool fcnChanged;
    // The 2d palette texture on the GPU for displaying the color map in the UI.
    GLuint paletteTex;

    // Select the provided color map specified by tfcnSelection. useOpacity
    // indicates if the transfer function's opacity values should be used if available
    // overwriting the user's set opacity data. This is done when loading from a file
    // to show the loaded tfcn, but not when switching from the preset picker.
    void setColorMap(const bool useOpacity);
    // Load up the preset color maps
    void loadColorMapPresets();
  };

  }// ::imgui3D
}// ::ospray

