// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

      //! \brief Initialize this node's value from given corresponding XML node
      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override;

     private:

      float interpolatedAlpha(const DataBuffer &alpha, float x);

      void calculateOpacities();
    };

  } // ::ospray::sg
} // ::ospray
