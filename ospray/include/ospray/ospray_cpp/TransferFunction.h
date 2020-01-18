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

#include "ManagedObject.h"

namespace ospray {
  namespace cpp {

    class TransferFunction
        : public ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>
    {
     public:
      TransferFunction(const std::string &type);
      TransferFunction(const TransferFunction &copy);
      TransferFunction(OSPTransferFunction existing = nullptr);
    };

    static_assert(sizeof(TransferFunction) == sizeof(OSPTransferFunction),
                  "cpp::TransferFunction can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline TransferFunction::TransferFunction(const std::string &type)
    {
      ospObject = ospNewTransferFunction(type.c_str());
    }

    inline TransferFunction::TransferFunction(const TransferFunction &copy)
        : ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>(
              copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline TransferFunction::TransferFunction(OSPTransferFunction existing)
        : ManagedObject<OSPTransferFunction, OSP_TRANSFER_FUNCTION>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::TransferFunction, OSP_TRANSFER_FUNCTION);

}  // namespace ospray
