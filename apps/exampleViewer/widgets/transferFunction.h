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

#include <ospray/ospray_cpp/TransferFunction.h>
#include <common/sg/transferFunction/TransferFunction.h>
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
    bool drawUI();
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
    void* widget;
    // The scenegraph transfer function being manipulated by this widget
    std::shared_ptr<sg::TransferFunction> transferFcn;
  };

}// ::ospray

