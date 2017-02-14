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

#include "Imgui3dExport.h"
#include <ospray/ospray_cpp/TransferFunction.h>
#include "../common/util/async_render_engine.h"

namespace ospray {

class OSPRAY_IMGUI3D_INTERFACE TransferFunction {
  // A line is made up of points sorted by x, its coordinates are
  // on the range [0, 1]
  struct Line {
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
    void move_point(const float &start_x, const ospcommon::vec2f &end);
    // Remove a point from the line, merging the two segments on either side
    void remove_point(const float &x);
  };

  // Lines for RGBA transfer function controls
  std::array<Line, 4> rgba_lines;
  // The line currently being edited
  int active_line;

  // Track if the function changed and must be re-uploaded.
  // We start by marking it changed to upload the initial palette
  bool fcn_changed;

  /* The 2d palette texture on the GPU for displaying the color map in the UI.
   */
  GLuint palette_tex;
  cpp::TransferFunction transferFcn;

  // Magic number to identify these files (it's VLFN in ASCII)
  const static int32_t MAGIC = 0x564c464e;

public:
  // The histogram for the volume data
  std::vector<size_t> histogram;

  TransferFunction();
  ~TransferFunction();
  TransferFunction(const TransferFunction&) = delete;
  TransferFunction& operator=(const TransferFunction&) = delete;
  /* Draw the transfer function editor widget
  */
  void drawUi();
  /* Render the transfer function to a 1D texture that can
   * be applied to volume data
   */
  void render(ospray::async_render_engine &renderEngine);
  /* Load the transfer function from a file
  */
  //bool load_fcn(const vl::FileName &file_name);

private:
  // Save/load the transfer function through the file dialog
  void save_fcn() const;
  void load_fcn();
};

}


