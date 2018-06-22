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

#include "../common/Node.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    /*! \brief a *tabulated* transfer function realized through
        uniformly spaced color and alpha values between which the
        value will be linearly interpolated (similar to a 1D texture
        for each) */
    struct OSPSG_INTERFACE TransferFunction : public sg::Node
    {
      TransferFunction();

      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;

      void loadParaViewTF(std::string fileName);

      float interpolateOpacity(const DataBuffer &controlPoints, float x);
      vec3f interpolateColor(const DataBuffer &controlPoints, float x);

      void updateChildDataValues();

    private:

      void computeOpacities();
      void computeColors();
    };

  } // ::ospray::sg
} // ::ospray
