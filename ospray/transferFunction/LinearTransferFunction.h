// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "LinearTransferFunction_ispc.h"
// std
#include <vector>

namespace ospray {

  /*! \brief A concrete implementation of the TransferFunction class for
    piecewise linear transfer functions.
  */
  class LinearTransferFunction : public TransferFunction 
  {
  public:

    //! Constructor.
    LinearTransferFunction() {}

    //! Destructor.
    virtual ~LinearTransferFunction();

    //! Allocate storage and populate the transfer function.
    virtual void commit();

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::LinearTransferFunction"); }

  protected:

    //! Data array that stores the color map.
    Ref<Data> colorValues;

    //! Data array that stores the opacity map.
    Ref<Data> opacityValues;

    //! Create the equivalent ISPC transfer function.
    virtual void createEquivalentISPC();

  };

} // ::ospray

