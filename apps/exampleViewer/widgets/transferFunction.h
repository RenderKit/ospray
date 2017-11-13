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

#include <ospray/ospray_cpp/TransferFunction.h>
#include <common/sg/transferFunction/TransferFunction.h>
#include <widgets/TransferFunctionWidget.h>
#include "Imgui3dExport.h"

namespace ospray {

  class OSPRAY_IMGUI3D_INTERFACE TransferFunction
  {
  public:

    TransferFunction(std::shared_ptr<sg::TransferFunction> tfn);
    ~TransferFunction();
    TransferFunction(const TransferFunction &t);
    TransferFunction& operator=(const TransferFunction &t);
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

  private:
    // Transfer function widget from 3rd party module
    tfn::tfn_widget::TransferFunctionWidget widget;
    // The scenegraph transfer function being manipulated by this widget
    std::shared_ptr<sg::TransferFunction> transferFcn;
  };

}// ::ospray

