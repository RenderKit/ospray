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
    class ImageOp : public ManagedObject_T<OSPImageOp>
    {
      public:

        ImageOp() = default;
        ImageOp(const std::string &type);
        ImageOp(const ImageOp &copy);
        ImageOp(OSPImageOp existing);
    };

    // Inlined function definitions ///////////////////////////////////////////////

    inline ImageOp::ImageOp(const std::string &type)
    {
      OSPImageOp c = ospNewImageOp(type.c_str());
      if (c) {
        ospObject = c;
      } else {
        throw std::runtime_error("Failed to create OSPImageOp!");
      }
    }

    inline ImageOp::ImageOp(const ImageOp &copy) :
      ManagedObject_T<OSPImageOp>(copy.handle())
    {
    }

    inline ImageOp::ImageOp(OSPImageOp existing) :
      ManagedObject_T<OSPImageOp>(existing)
    {
    }


  }// namespace cpp
}// namespace ospray
