// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospray
#include "common/Data.h"
#include "transferFunction/TransferFunction.h"

namespace ospray {

  /*! \brief A concrete implementation of the TransferFunction class for
    piecewise linear transfer functions.
  */
  struct OSPRAY_SDK_INTERFACE LinearTransferFunction : public TransferFunction
  {
    LinearTransferFunction();
    virtual ~LinearTransferFunction() override;

    virtual void commit() override;

    virtual std::string toString() const override;

  private:

    //! Data array that stores the color map.
    Ref<Data> colorValues;

    //! Data array that stores the opacity map.
    Ref<Data> opacityValues;
  };

} // ::ospray

